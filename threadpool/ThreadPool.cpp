#include "ThreadPool.hpp"

#include <math.h>

namespace cshr {
sptr<ThreadPoolImpl> ThreadPoolImpl::instance;
ThreadPoolImpl::ThreadPoolImpl() {
  cshrlog("ThreadPoolImp initiated.\n");
  sem_init(&taskSem, false, 0);
  int cpuNumber = (int)sysconf(_SC_NPROCESSORS_CONF);
#ifdef __cshr_debug__
  cpuNumber = cpuNumber < 8 ? 8 : cpuNumber;
#endif
  cshrlog("cpu number: %d\n", cpuNumber);
  for (int i = 0; i < cpuNumber; ++i) {
    auto func = [&, i]() { this->WorkThreadMain(i); };
    workThreads.push_back(std::make_unique<Worker>(i, std::thread(func)));
  }
  cshrlog("ThreadPoolImp initiated over.\n");
}

ThreadPoolImpl::~ThreadPoolImpl() {
  running.store(false);
  for (int i = 0; i < workThreads.size(); ++i) {
    sem_post(&taskSem);
  }
  for (const auto &workThread : workThreads) {
    workThread->thread_entry.join();
  }
  sem_destroy(&taskSem);
  cshrlog("ThreadPoolImpl destroy.\n");
}
cshr::sptr<ThreadPoolImpl> ThreadPoolImpl::GetInstance() {
  if (!instance) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    if (!instance) {
      instance = new ThreadPoolImpl();
    }
  }
  cshrlog("ThreadPoolImpl gets singleton.\n");
  return instance;
}
void ThreadPoolImpl::postTask(ThreadPoolTask task, string name) {
  {
    std::lock_guard<std::mutex> lock(tasksMutex);
    tasks.push({.task = task, .name = name});
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
      task = tasks.front();
      tasks.pop();
    }
    cshrlog("Work(%d) %s starts\n", id, task.name.data());
    task.task();
    cshrlog("Work(%d) %s ends\n", id, task.name.data());
  }
  cshrlog("WorkThread %d stops\n", id);
}

};  // namespace cshr