#pragma once
#include <chrono>
#include <future>
#include <vector>

#include "liburing/liburing.h"
namespace Cshr::Service {
using namespace std::chrono;
// AioFlush接口
// 把caller提供的buffer全部flush到对应的fd中
class AioFlushService {
  using IoVec = struct iovec;

 public:
  int wait_for_complete(std::chrono::microseconds = 0ms);
  inline std::optional<std::future<bool>> flush(
      const std::vector<IoVec> &strVec, int fd) {
    return aio_flush(strVec, fd);
  }
  virtual std::optional<std::future<bool>> aio_flush(const std::vector<IoVec> &,
                                                     int) = 0;
  AioFlushService() {}
  virtual ~AioFlushService() {}
};

// IoUring的AioFlush实现
// 实现涉及到liburing库的api接口
constexpr size_t uringSize = 1024;
class IoUringAioFlushService : public AioFlushService {
  using IoUring = struct io_uring;
  using CompletionQueryEntry = struct io_uring_cqe;
  using CPtr = CompletionQueryEntry *;
  using SubmissionQueryEntry = struct io_uring_sqe;
  using SPtr = SubmissionQueryEntry *;
  using IoVec = struct iovec;
  // FlushRequest
  // 注册进SQE的对象，最后会由CQE返回得到
  // 由于liburing更接近于C-style，这里很难用RAII
  // 需要注意的是，IoVec里包括的所有buffer的生命期是caller自己控制的
  // 这样是为了减少拷贝，caller需要自己保证提交的buffer在处理完成以前不会被回收
  struct FlushRequest {
    std::promise<bool> mPromise;  // 每一个request都会对应一个是否成功的promise
    int mFd;                      // Flush到的文件描述符
    std::vector<IoVec> mIovecs;
    // FlushRequest不负责各个buffer的生命期, 所以移动构造失效
    FlushRequest(std::vector<IoVec> &&, int fd) = delete;
    FlushRequest(const std::vector<IoVec> &iovecs, int fd)
        : mFd(fd), mIovecs(iovecs) {}
  };
  // SQE的flusher对象
  // 用于flush一个FlushRequest到sqe数组中
  struct SqeFlusher {
   private:
    // FlushRequest的生命期由SqeFlusher创造
    // 但是销毁由CqeCleaner进行
    FlushRequest *mRequest;
    bool isFlush;

   public:
    // flush仅保证把request放入数组，不决定是否提交到内核
    std::future<bool> flush(SPtr sqe) {
      if (mRequest && isFlush) return mRequest->mPromise.get_future();
      io_uring_sqe_set_data(sqe, mRequest);
      io_uring_prep_writev(sqe, mRequest->mFd, mRequest->mIovecs.data(),
                           mRequest->mIovecs.size(), 0);
      isFlush = true;
      return mRequest->mPromise.get_future();
    }

   public:
    SqeFlusher(const std::vector<IoVec> &strVec, int fd)
        : mRequest(new FlushRequest(strVec, fd)), isFlush(false) {}
    SqeFlusher(const SqeFlusher &flusher) = delete;
    SqeFlusher(SqeFlusher &&flusher) {
      mRequest = flusher.mRequest;
      isFlush = flusher.isFlush;
      flusher.mRequest = nullptr;
    }
    SqeFlusher &operator=(SqeFlusher &&flusher) {
      mRequest = flusher.mRequest;
      isFlush = flusher.isFlush;
      flusher.mRequest = nullptr;
      return *this;
    }
    ~SqeFlusher() noexcept {
      // 如果没有flush过request, 那么request的生命期需要由flusher结束
      if (mRequest && !isFlush) {
        mRequest->mPromise.set_value(false);
        delete mRequest;
      }
    }
  };

  struct CqeCleaner {
   private:
    FlushRequest *mRequest;

   public:
    CqeCleaner(FlushRequest *request) : mRequest(request) {}
    ~CqeCleaner() noexcept {
      if (mRequest) delete mRequest;
    }

   private:
  };

 protected:
  SqeFlusher make_flush_request(const std::vector<IoVec> &strVec, int fd) {
    return SqeFlusher(strVec, fd);
  }

  CqeCleaner make_cqe_cleaner() {}

 public:
  std::optional<std::future<bool>> aio_flush(const std::vector<IoVec> &strVec,
                                             int fd) {
    if (strVec.empty()) return std::nullopt;
    SPtr sqe = get_sqe_ptr();
    auto future = make_flush_request(strVec, fd).flush(sqe);
    non_sync_submit();
    return future;
  }
  IoUringAioFlushService(int fd) { io_uring_queue_init(uringSize, &mRing, 0); }
  virtual ~IoUringAioFlushService() override { io_uring_queue_exit(&mRing); }

 private:
  SPtr get_sqe_ptr() { return io_uring_get_sqe(&mRing); }
  FlushRequest *get_cqe_request_ptr() {
    CPtr cqe;
    int ret = io_uring_wait_cqe(&mRing, &cqe);
    assert(ret >= 0);
    auto fr = reinterpret_cast<FlushRequest *>(cqe->user_data);
    fr->mPromise.set_value(!(cqe->res < 0));
    io_uring_cqe_seen(&mRing, cqe);
  }
  void sync_submit() { io_uring_submit(&mRing); }
  void non_sync_submit() {
    if (++mSqeReadyNumber >= submitHighWaterMark) sync_submit();
  }

 private:
  constexpr static size_t submitHighWaterMark = 0; // 目前每次都sync submit
  IoUring mRing;
  size_t mSqeReadyNumber;
  size_t mCqeReadyNumber;
};
}  // namespace Cshr::Service