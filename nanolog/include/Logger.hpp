#pragma once
#include <array>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "LoggerInfo.hpp"
#include "Service.hpp"
#include "SwapBuffer.hpp"
#include "Sync.hpp"
namespace Cshr::LoggerInternal {
using namespace Cshr::Logger;
template <typename... Types, unsigned N, unsigned NType>
void log(int &logId, const char *fileName, const int lineNum,
         const LogLevel &serverity, const char (&format)[N],
         const std::array<ParamType, NType> &paramTypes, Types... Args) {}
class Logger {
  class LoggerBuffer;

 public:
  // 修改log file
  static bool set_log_file(const char *fileName) {
    return runtimeLoggerSingleton.set_log_file_internal(fileName);
  }
  // 修改当前logger优先级
  static bool set_log_level(LogLevel setLevel) {
    setLevel = std::clamp(setLevel, static_cast<LogLevel>(0),
                          static_cast<LogLevel>(LOG_TYPE_NUM - 1));
    return runtimeLoggerSingleton.set_log_level_internal(setLevel);
  }
  // 获取当前logger优先级
  static LogLevel get_log_level() {
    return runtimeLoggerSingleton.get_log_level_internal();
  }
  // 注册log实例
  static void register_new_logger_entity(int32_t &loggerId,
                                         LoggerInfo LoggerInfo) {
    runtimeLoggerSingleton.register_new_logger_entity_internal(loggerId,
                                                               LoggerInfo);
  }
  // 申请一段buffer缓存
  static inline char *ask_for_buffer_with_size(size_t nbytes) {
    if (!mThreadLoggerBufferPtr) {
      runtimeLoggerSingleton.allocate_logger_buffer();
    }
    return mThreadLoggerBufferPtr->allocate_buffer_with_size(nbytes);
  }
  // 提交申请的buffer缓存
  static inline void commit_buffer_with_size(size_t nbytes) {
    mThreadLoggerBufferPtr->commit_buffer_with_size(nbytes);
  }
  // 类似于fsync功能
  static void sync() { runtimeLoggerSingleton.sync_internal(); }

 private:
  void IoBackgroundThreadFunction();
  void io_change_sync_status();
  bool set_log_file_internal(const char *);
  bool set_log_level_internal(LogLevel setLevel) {
    mCurLogLevel = setLevel;
    return true;
  }
  LogLevel get_log_level_internal() { return mCurLogLevel; }
  // 注册logger实例
  void register_new_logger_entity_internal(int32_t &loggerId,
                                           LoggerInfo loggerInfo) {
    std::lock_guard lock(mLoggerRegisterMutex);
    if (loggerId != Cshr::Logger::loggerIdUnsigned) return;
    loggerId = static_cast<int32_t>(mLoggerInfoVector.size());
    mLoggerInfoVector.push_back(std::move(loggerInfo));
  }
  void sync_internal();

 private:
  void allocate_logger_buffer() {
    assert(!mThreadLoggerBufferPtr);
    LoggerBuffer *buffer;
    {
      std::unique_lock lock(mThreadBufferMutex);
      uint32_t bufferId = mAllocatedBufferTimes++;
      lock.unlock();
      LoggerBuffer *buffer = new LoggerBuffer(bufferId);
      lock.lock();
      mThreadLoggerBufferPtrs.emplace_back(buffer);
    }
    mThreadLoggerBufferPtr = buffer;
  }

 private:
  // LoggerBuffer
  // Logger各个线程自己的Buffer, 对于IO线程来说可见一个全局的LoggerBuffer数组
  class LoggerBuffer {
    friend class Logger;  // 为了让IO的Logger线程可以访问其他线程的LoggerBuffer
    friend class std::unique_ptr<LoggerBuffer>;  // LoggerBuffer的友元智能指针
    using LoggerBufferDeleter =
        decltype(std::declval<std::unique_ptr<LoggerBuffer>>().get_deleter());
    friend LoggerBufferDeleter;  // LoggerBuffer的友元删除器
   public:
    ~LoggerBuffer() {}
   private:
    explicit LoggerBuffer(uint32_t bufferId)
        : mBufferId(bufferId),
          mAllocationCount(0),
          mBlockProducerTimes(0),
          mFreeSpace(0),
          mProducePos(mBuffer),
          mBufferDataEnd(mBuffer + sizeof(mBuffer)),
          mConsumePos(mBuffer) {}
    DISALLOW_COPY_AND_ASSIGN(LoggerBuffer);

   private:
    uint32_t get_id() { return mBufferId; }
    // 请求分配nbytes大小的缓冲块
    char *allocate_buffer_with_size(size_t nbytes) {
      ++mAllocationCount;
      if (nbytes < mFreeSpace) return mProducePos;
      return update_and_allocate_buffer(nbytes);
    }
    void commit_buffer_with_size(size_t nbytes) {
      assert(nbytes < mFreeSpace);
      assert(mProducePos + nbytes <
             mBuffer + Cshr::Logger::defaultLoggerBufferSize);
      Cshr::Sync::store_barrier();
      mFreeSpace -= nbytes;
      mProducePos += nbytes;
    }
    inline void consume_buffer_with_size(size_t nbytes) {
      fetch_buffer_with_size(nbytes);
    }
    void fetch_buffer_with_size(size_t nbytes) {
      Cshr::Sync::load_barrier();
      mConsumePos += nbytes;
    }
    char *peek_buffer_at_least(uint64_t atLeastSize) {
      uint64_t totalBytes;
      char *ret = peek_full_available_buffer(&totalBytes);
      if (atLeastSize <= totalBytes) return ret;
      return nullptr;
    }
    char *peek_full_available_buffer(uint64_t *totalBytes) {
      char *cachedProducePos = mProducePos;
      if (cachedProducePos < mConsumePos) {
        Cshr::Sync::load_barrier();
        *totalBytes = mBufferDataEnd - mConsumePos;
        if (*totalBytes > 0) return mConsumePos;
        // 这一轮的数据已经读完了
        mConsumePos = mBuffer;
      }
      *totalBytes = cachedProducePos - mConsumePos;
      return mConsumePos;
    }
    // 这块内存已经没有用了
    bool waiting_for_destroy() {
      return mDiscarded && mConsumePos == mProducePos;
    }
    char *update_and_allocate_buffer(size_t nbytes, bool canBlocking = true);

   private:
    uint32_t mBufferId;  // bufferId为每个线程第一次创建Logger
    uint64_t mAllocationCount;  // 统计量，用于统计LoggerBuffer被produce的次数
    uint32_t mBlockProducerTimes;  // 统计量，用于统计LoggerBuffer被阻塞的次数
    bool mDiscarded{false};  // 丢弃标记，释放buffer的行为由IO线程来做
    char mBuffer[Cshr::Logger::defaultLoggerBufferSize];  // Buffer实体
    uint64_t mFreeSpace;  // 记录的目前可用空间, 因为cache可能没更新,
                          // 所以是一个最小阈值
    char *mProducePos;  // produce 下标
    // 每一圈的buffer最终位置(出现的原因是存在produce最后一块缓冲不够分配，
    // 此时这一块缓冲需要丢弃，否则可能无限期阻塞)
    char *mBufferDataEnd;
    union {
      char __mCacheLineSpacer[Cshr::Logger::cacheLineBytesSize];
    };
    char *mConsumePos;  // cosume下标
  };

 private:
  Logger();
  ~Logger();
  DISALLOW_COPY_AND_ASSIGN(Logger);

 private:
  static thread_local LoggerBuffer
      *mThreadLoggerBufferPtr;  // 每个线程独有的Buffer
  static Logger
      runtimeLoggerSingleton;  // 全局单例，负责各个线程的注册以及buffer相关请求以及IO主线程的请求
 private:
  std::shared_ptr<Cshr::Service::AioFlushService> mService;  // 异步服务
  Cshr::Buffer::SwapBuffer mSwapBuffer;                 // swap buffer
  std::mutex mLoggerRegisterMutex;
  std::mutex mThreadBufferMutex;
  std::mutex mSyncMutex;
  std::condition_variable mSyncCompleteVariable;
  std::condition_variable mWorkAddedVariable;
  uint32_t mAllocatedBufferTimes;
  std::vector<LoggerInfo> mLoggerInfoVector;
  std::vector<std::unique_ptr<LoggerBuffer>> mThreadLoggerBufferPtrs;
  LogLevel mCurLogLevel;
  uint64_t mConsumeBytesCount{0};
  uint64_t mConsumeCycleCount{0};
  bool mIsAioing{false};
  uint64_t mAioBytesCount{0};
  uint64_t mAioCompleteTimesCount{0};
  uint64_t mPaddingBytesCount{0};
  uint64_t mLogNumberCount{0};
  enum {
    SYNCPENDING,  // 有sync请求
    DOUBLECHECK,  // sync会再次对所有buffer检查确认没有未完成buffer
    AIOPENDING,  // 处于等待AIO阶段
    COMPLETED    // 没有sync请求或请求已完成
  } mSyncStatus;
  enum {
    RESTART,        // 新启一个背景线程
    RUNNING,        // 背景线程运行
    INTERRUPTIBLE,  // 背景线程睡眠
    STOPPING        // 背景线程已关闭
  } mBGThreadStatus;
  std::thread mInternalIoThread;
  int mFileFd;  // unix 文件描述符
  int mCoreId;  // 最后一次运行的cpu核
};
}  // namespace Cshr::LoggerInternal
