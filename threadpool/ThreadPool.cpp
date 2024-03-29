#include "ThreadPool.hpp"

namespace cshr {
typename ThreadPoolImpl::sptr ThreadPoolImpl::instance_;
ThreadPoolImpl::ThreadPoolImpl() {
  cshrlog("ThreadPoolImp instance address: %p\n", this);
  cshrlog("ThreadPoolImp initiated.\n");
  sem_init(&taskSem, false, 0);
  int cpuNumber = (int)sysconf(_SC_NPROCESSORS_CONF);
#ifdef __cshr_debug__
  constexpr int debug_cpu_number = 16;
  cpuNumber = cpuNumber < debug_cpu_number ? debug_cpu_number : cpuNumber;
#endif
  cshrlog("cpu number: %d\n", cpuNumber);
  for (int i = 0; i < cpuNumber; ++i) {
    auto func = [&, i]() { this->WorkThreadMain(i); };
    workThreads.push_back(std::make_unique<Worker>(i, std::thread(func)));
  }
  cshrlog("ThreadPoolImp initiated over.\n");
}

ThreadPoolImpl::~ThreadPoolImpl() {
  cshrlog("ThreadPoolImpl destroying.\n");
  running.store(false);
  for (int i = 0; i < workThreads.size(); ++i) {
    sem_post(&taskSem);
  }
  for (const auto &workThread : workThreads) {
    workThread->thread_entry.join();
  }
  sem_destroy(&taskSem);
  cshrlog("ThreadPool Clean left jobs.\n");
  while(!tasks.empty()) {
    tasks.front().task();
    tasks.pop();
  }
  cshrlog("ThreadPoolImpl destroyed.\n");
}
typename ThreadPoolImpl::sptr ThreadPoolImpl::GetInstance() {
  cshrlog("ThreadPoolImpl::GetInstance\n");
  if (!instance_) {
    static std::mutex mutex;
    {
      std::lock_guard<std::mutex> lock(mutex);
      if (!instance_) {
        instance_ = new ThreadPoolImpl();
      }
    }
  }
  cshrlog("ThreadPoolImpl gets singleton.\n");
  return instance_;
}
void ThreadPoolImpl::postTask(ThreadPoolTask &&task, string name) {
  {
    std::lock_guard<std::mutex> lock(tasksMutex);
    tasks.emplace(std::move(task), name);
  }
  sem_post(&taskSem);
}

void ThreadPoolImpl::WorkThreadMain(int id) {
  cshrlog("WorkThread %d starts\n", id);
  while (true) {
    sem_wait(&taskSem);
    if (!running.load()) {
      break;
    }
    Task task;
    {
      std::lock_guard<std::mutex> lock(tasksMutex);
      task = std::move(tasks.front());
      tasks.pop();
    }
    cshrlog("Work(%d) %s starts\n", id, task.name.data());
    task.task();
    cshrlog("Work(%d) %s ends\n", id, task.name.data());
  }
  cshrlog("WorkThread %d stops\n", id);
}

};  // namespace cshr