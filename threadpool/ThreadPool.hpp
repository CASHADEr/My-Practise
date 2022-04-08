#pragma once
#include <semaphore.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "../log/cshrlog.hpp"
#include "../refbase/refbase.hpp"

namespace cshr {
using ThreadPoolTask = std::function<void()>;

class ThreadPoolImpl;
template <typename _Ty = ThreadPoolImpl>
class ThreadPool : public RefBase {
  using string = std::string;
  using sptr = cshr::sptr<ThreadPool<_Ty>>;

 public:
  static sptr GetInstance() { 
    cshrlog("ThreadPool::GetInstance\n");
    return _Ty::GetInstance();
  }
  virtual void postTask(ThreadPoolTask task, string name = "") = 0;
  ~ThreadPool() override {}
};

class ThreadPoolImpl : public ThreadPool<ThreadPoolImpl> {
  using string = std::string;
  using sptr = cshr::sptr<ThreadPoolImpl>;
 public:
  static sptr GetInstance();
  virtual void postTask(ThreadPoolTask task, string name) override;
  ~ThreadPoolImpl() override;

 private:
  ThreadPoolImpl();
  void WorkThreadMain(int id);

 private:
  static sptr instance_;
  struct Worker {
    int id;
    std::thread thread_entry;
    Worker() = default;
    Worker(int id, std::thread&& thread_entry)
        : id(id), thread_entry(std::move(thread_entry)) {}
  };
  struct Task {
    ThreadPoolTask task;
    string name;
  };
  using queue = std::queue<Task>;
  std::queue<Task> tasks;
  std::mutex tasksMutex;
  std::vector<std::unique_ptr<Worker>> workThreads;
  sem_t taskSem;
  std::atomic<bool> running{true};
};
};  // namespace cshr