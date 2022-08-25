#pragma once
#include <semaphore.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

#include "../log/cshrlog.hpp"
#include "../refbase/refbase.hpp"
#include "./TaskWrapper.hpp"

namespace cshr {
using ThreadPoolTask = TaskWrapper;

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
  template <typename Fn, typename... Types>
  auto emplace(Fn &&fn, Types &&...args) {
    return emplace("", std::forward<Fn>(fn), std::forward<Types>(args)...);
  }
  template <size_t N, typename Fn, typename... Types>
  auto emplace(const char (&name)[N], Fn &&fn, Types &&...args) {
    return emplace(std::string(name), std::forward<Fn>(fn),
                   std::forward<Types>(args)...);
  }
  template <typename Fn, typename... Types>
  auto emplace(string name, Fn &&fn, Types &&...args)
      -> std::future<std::invoke_result_t<Fn, Types...>> {
    using ret_type = std::invoke_result_t<Fn, Types...>;
    auto taskFn = std::packaged_task<ret_type()>(
        [=]() mutable -> ret_type { return fn(std::forward<Types>(args)...); });
    auto res = taskFn.get_future();
    postTask(std::move(taskFn), name);
    return res;
  }
  virtual void postTask(ThreadPoolTask &&task, string name = "") = 0;
  ~ThreadPool() override {}
};

class ThreadPoolImpl : public ThreadPool<ThreadPoolImpl> {
  using string = std::string;
  using sptr = cshr::sptr<ThreadPoolImpl>;

 public:
  static sptr GetInstance();
  virtual void postTask(ThreadPoolTask &&task, string name) override;
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
    Worker(int id, std::thread &&thread_entry)
        : id(id), thread_entry(std::move(thread_entry)) {}
  };
  struct Task {
    ThreadPoolTask task;
    string name;
    Task() = default;
    Task(ThreadPoolTask &&task_, const string &name_)
        : task(std::move(task_)), name(name_) {}
    Task &operator=(Task &&task_) {
      task = std::move(task_.task);
      name = task_.name;
      return *this;
    }
  };
  using queue = std::queue<Task>;
  std::queue<Task> tasks;
  std::mutex tasksMutex;
  std::vector<std::unique_ptr<Worker>> workThreads;
  sem_t taskSem;
  std::atomic<bool> running{true};
};
};  // namespace cshr