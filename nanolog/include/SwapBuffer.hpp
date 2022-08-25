#pragma once
#include <chrono>
#include <cstring>
#include <future>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>
namespace Cshr::Buffer {
constexpr size_t loadingIovecCapacity = 1 << 12;
constexpr uint64_t defaultBufferCapacityLimit = loadingIovecCapacity << 2;
constexpr uint64_t swapBufferHighWaterMark = 0.75 * defaultBufferCapacityLimit;
class SwapBuffer {
  using IoVec = struct iovec;

 public:
  SwapBuffer() = default;
  ~SwapBuffer() = default;
  bool empty() { return size() == 0; }
  bool size() { return mLoadedBufferSize; }
  bool full() { return mIsFull; }
  template <typename Rep, typename Period>
  bool wait_for_aio_timeout(const std::chrono::duration<Rep, Period> &timeout) {
    using namespace std::chrono;
    if (timeout == 0ms) {
      mFlushFuture->wait();
      return true;
    }
    auto status = mFlushFuture->wait_for(timeout);
    return status == std::future_status::ready
  }
  // 把当前的buffer提交给一个异步服务
  template <typename Service>
  bool aio_flush(Service &service,
                 std::enable_if_t<
                     std::is_base_of_v<Cshr::Service::AioFlushService, Service>,
                     void *> = nullptr) {
    if (mFlushFuture) mFlushFuture->get();  // 如果还有flushing，一直阻塞
    std::vector<std::string> tmp;           // 用于销毁flush过的buffer
    tmp.swap(mFlushingBufferVector);
    mFlushingBufferVector.swap(mLoadingBufferVector);
    mFlushFuture = service.flush(mFlushingBufferVector);
    return true;
  }
  // 读取新的log提交
  uint64_t load_buffer_from(const char *message, uint64_t msz) {
    if (msz + mLoadedBufferSize < defaultBufferCapacityLimit) {
      mLoadedBufferSize += msz;
      if (mLoadedBufferSize >= swapBufferHighWaterMark) mIsFull = true;
      uint64_t remaining = msz;
      while (remaining) {
        uint64_t consume =
            std::min(remaining, loadingIovecCapacity - mCurLoadingPos);
        ::memcpy(mLoadingBufferVector.back().iov_base + mCurLoadingPos, message,
                 consume);
        remaining -= consume;
        mCurLoadingPos += consume;
        if (remaining && mCurLoadingPos == loadingIovecCapacity) {
          allocate_new_buffer_block();
          ++mCurLoadingBufferIndex;
          mCurLoadingPos = 0;
        }
      }
      return msz;
    }
    return 0;
  }

 private:
  void allocate_new_buffer_block() {
    void *buffer = ::malloc(loadingIovecCapacity * sizeof(char));
    mLoadingBufferVector.emplace_back();
    auto &iovec = mLoadingBufferVector.back();
    iovec.iov_base = buffer;
    iovec.iov_len = 0;
  }

 private:
  uint64_t mLoadedBufferSize{0};  // 标记buffer目前的数据大小
  bool mIsFull{false};            // 标记buffer是否填满
  std::optional<std::future<bool>> mFlushFuture;  // Flushing Future
  uint64_t mCurLoadingPos{0};
  size_t mCurLoadingBufferIndex{0};
  std::vector<IoVec> mLoadingBufferVector;   // 用于填充用的buffer
  std::vector<IoVec> mFlushingBufferVector;  // 用于提交的buffer
};
}  // namespace Cshr::Buffer