
#include "include/Logger.hpp"

#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

#include <cassert>
#include <chrono>

#include "Timer.hpp"
#include "include/TypeUtil.hpp"
namespace Cshr::LoggerInternal {
thread_local Logger::LoggerBuffer *Logger::mThreadLoggerBufferPtr;
/**
 * @brief update and allocate buffer
 * 意味着要更新BGThread对其consumePos作出的修改，从而扩大freeSpace
 * 这也意味着要更新缓存，那么会影响性能(Nanolog对其形容为Slow Allocate)
 * 同时如果更新后还是不够那么就会导致blocking，是否blocking取决于对应的blocking参数
 *
 * @param nbytes 分配的缓冲区大小
 * @return char* 分配的缓冲区首地址
 */
char *Logger::LoggerBuffer::update_and_allocate_buffer(size_t nbytes,
                                                       bool canBlocking) {
  const char *bufferEnd = mBuffer + Cshr::Logger::defaultLoggerBufferSize;
  // 这里会一直自选阻塞到FreeSpace足够
  // 自选的原因就是为了避免涉及到锁
  // 纯无锁队列的设计如果再加上阻塞，那就没有意义了
  while (mFreeSpace <= nbytes) {
    auto cacheConsumerPos = mConsumePos;
    // 这里要考虑到缓冲区反转
    if (cacheConsumerPos <= mProducePos) {
      mFreeSpace = bufferEnd - mProducePos;
      if (mFreeSpace > nbytes) break;
      // 空间不够，最后一小块缓冲区直接舍弃
      mBufferDataEnd = mProducePos;
      if (cacheConsumerPos != mBuffer) {
        // 屏障保护BufferDataEnd和ProducePos的更新顺序
        Cshr::Sync::store_barrier();
        mProducePos = mBuffer;
        mFreeSpace = cacheConsumerPos - mProducePos;
      }
    } else {
      mFreeSpace = cacheConsumerPos - mProducePos;
    }
    if (!canBlocking && mFreeSpace <= nbytes) return nullptr;
  }
  ++mBlockProducerTimes;
  return mProducePos;
}
Logger Logger::runtimeLoggerSingleton;
Logger::Logger()
    : mService(new Cshr::Service::IoUringAioFlushService()),
      mSwapBuffer(),
      mBGThreadStatus(RESTART),
      mInternalIoThread(&Logger::IoBackgroundThreadFunction, this),
      mAllocatedBufferTimes(0),
      mFileFd(-1),
      mCoreId(-1) {
  const char *fileName = Cshr::Logger::defaultLogFilePath;
  mFileFd = open(fileName, Cshr::Logger::defaultLogOpenSetting);
}
Logger::~Logger() {}
bool Logger::set_log_file_internal(const char *fileName) {
  int newFd;
  bool accessible =
      access(fileName, F_OK) || access(fileName, R_OK | W_OK) == 0;
  if (!accessible) goto err;
  newFd = open(fileName, Cshr::Logger::defaultLogOpenSetting,
               Cshr::Logger::defaultLogOpenMode);
  if (newFd < 0) goto err;
  // 之前的logger记录全部写入前一个文件中
  Logger::sync();
  // 重启一个背景线程
  {
    std::unique_lock lock(mSyncMutex);
    mBGThreadStatus = STOPPING;  // 背景线程关闭标记
    mWorkAddedVariable.notify_all();
    lock.unlock();
    if (mInternalIoThread.joinable()) mInternalIoThread.join();
    lock.lock();
    mBGThreadStatus = RESTART;  // 背景线程重启标记
  }
  if (mFileFd >= 0) close(mFileFd);
  mFileFd = newFd;
  mInternalIoThread = std::thread(&Logger::IoBackgroundThreadFunction, this);
  return true;
err:
  return false;
}
// Logger的核心工作函数
void Logger::IoBackgroundThreadFunction() {
  using namespace Cshr::Timer;
  using namespace Cshr::Util;
  using namespace std::chrono;
  // 设置buffer游标, 用于轮训遍历buffer
  size_t lastNonEmptyBufferIndexChecked = 0;
  uint64_t cycleStart = Timer::get_cpu_cycle();
  uint64_t cycleThreadStart = cycleStart;
  // 设置LoggerInfo的read view, 这样能够避免加锁(MVCC)
  std::vector<LoggerInfo> shadowLoggerInfoVector;
  // 设置当前状态为正在运行
  {
    std::lock_guard lock(mSyncMutex);
    mBGThreadStatus = RUNNING;
  }
  bool isNewRound = false;
  while (mBGThreadStatus != STOPPING) {
    mCoreId = sched_getcpu();  // 获取当前的cpu核心id
    // 当前这一轮循环的字节消耗计数
    uint64_t consumedBytesInCurrentIteration = 0;
    {
      std::unique_lock bufferLock(mThreadBufferMutex);
      size_t curLoggerBufferIndex = lastNonEmptyBufferIndexChecked;
      while (!mSwapBuffer.full() && !mThreadLoggerBufferPtrs.empty()) {
        LoggerBuffer *curBufferPtr =
            mThreadLoggerBufferPtrs[curLoggerBufferIndex].get();
        uint64_t peekBytes = 0;
        char *availableBufferHead =
            curBufferPtr->peek_full_available_buffer(&peekBytes);
        if (availableBufferHead) {
          auto io_start_time = get_cpu_cycle();
          bufferLock.unlock();
          uint32_t remaining = static_down_cast<uint32_t>(peekBytes);
          while (remaining > 0) {
            // 在buffer里获取一段有效的message log
            uint64_t consumedLogsSize = mSwapBuffer.load_buffer_from(
                availableBufferHead + peekBytes - remaining, remaining);
            // consume失败说明aio_buffer已经满了
            if (consumedLogsSize == 0) {
              lastNonEmptyBufferIndexChecked = curLoggerBufferIndex;
              break;
            }
            isNewRound = false;
            remaining -= static_down_cast<uint32_t>(consumedLogsSize);
            curBufferPtr->consume_buffer_with_size(consumedLogsSize);
            mConsumeBytesCount += consumedLogsSize;
            consumedBytesInCurrentIteration += consumedLogsSize;
          }
          mConsumeCycleCount += get_cpu_cycle() - io_start_time;
          bufferLock.lock();
        } else {
          // 说明这个buffer的线程在获取之前没有写的任务
          if (curBufferPtr->waiting_for_destroy()) {
            {
              auto itr = mThreadLoggerBufferPtrs.begin() + curLoggerBufferIndex;
              mThreadLoggerBufferPtrs.erase(itr);
            }
            if (mThreadLoggerBufferPtrs.empty()) {
              lastNonEmptyBufferIndexChecked = curLoggerBufferIndex = 0;
              isNewRound = true;
              break;
            }
            if (lastNonEmptyBufferIndexChecked >= curLoggerBufferIndex &&
                lastNonEmptyBufferIndexChecked > 0) {
              --lastNonEmptyBufferIndexChecked;
            }
            --curLoggerBufferIndex;
          }
        }
        ++curLoggerBufferIndex;
        curLoggerBufferIndex %= mThreadLoggerBufferPtrs.size();
        if (curLoggerBufferIndex == 0) isNewRound = true;
        // 这表示一轮下来没有任何buffer提供了数据
        if (curLoggerBufferIndex == lastNonEmptyBufferIndexChecked) break;
      }
    }

  // aio buffer 为空的时候，需要判断io线程目前的同步状态
  // 这段操作要优先于aio判断，原因是sync状态在可能正好要切换到double check
  // 这样sync阶段在aio还在进行的时候就已经可以切换到double check
  empty_aio_buffer_stage:
    if (mSwapBuffer.empty()) {
      std::unique_lock synLock(mSyncMutex);
      if (mSyncStatus == SYNCPENDING) {
        // 此时数组中还可能存在有效的buffer log
        // 需要double check
        mSyncStatus = DOUBLECHECK;
        continue;
      }
      if (mSyncStatus == DOUBLECHECK) {
        mSyncStatus = mIsAioing ? AIOPENDING : COMPLETED;
      }
      if (mSyncStatus == COMPLETED) {
        mSyncCompleteVariable.notify_one();  // 还没弄懂为什么只notify一个
        mWorkAddedVariable.wait_for(
            synLock, std::chrono::microseconds(
                         Cshr::Logger::ioThreadSleepTimeIntervalMicroseconds));
        cycleStart = Cshr::Timer::get_cpu_cycle();
      }
    }

  // 目前正在进行aio
  // 那么此时需要等到aio完成或者重新回到读buffer的状态
  aio_waiting_stage:
    if (mIsAioing) {
      // aio buffer是满的，现在只能等aio结束
      if (mSwapBuffer.full()) {
        mSwapBuffer.wait_for_aio_timeout(0ms);
        cycleStart = get_cpu_cycle();
      } else {
        // swapbuffer没有满，根据是否处于aiopending状态
        // 来选择是否持续阻塞
        bool isAioPending = false;
        {
          std::lock_guard lock(mSyncMutex);
          isAioPending = mSyncStatus == AIOPENDING;
        }
        auto sleepTime = isAioPending ? 0 : Cshr::Logger::ioThreadBlockedByAIOMicroseconds;
        bool aioComplete = false;
        if (consumedBytesInCurrentIteration == 0 && sleepTime > 0) {
          aioComplete = mSwapBuffer.wait_for_aio_timeout(
              std::chrono::milliseconds(sleepTime));
        }
        if (!aioComplete) continue; // 如果还不能用就返回继续去读缓冲
      }
      // 这里必须执行完毕
      ++mAioCompleteTimesCount;
      mIsAioing = false;
      {
        std::lock_guard synLock(mSyncMutex);
        if (mSyncStatus == AIOPENDING) {
          mSyncStatus = COMPLETED;
          mSyncCompleteVariable.notify_one();
        }
      }
    }
  aio_stage:
    if (mSwapBuffer.empty()) continue;
    mAioBytesCount += mSwapBuffer.size();
    mSwapBuffer.aio_flush(*mService);
    mIsAioing = true;
  }
}
void Logger::sync_internal() {
  std::unique_lock lock(mSyncMutex);
  if (mSyncStatus != COMPLETED) return;
  mSyncStatus = SYNCPENDING;
  mWorkAddedVariable.notify_all();
  mSyncCompleteVariable.wait(lock);
}
}  // namespace Cshr::LoggerInternal
