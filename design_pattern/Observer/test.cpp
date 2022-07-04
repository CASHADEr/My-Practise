#include <functional>
#include <mutex>
#include <vector>
#include <iostream>

#include "Observer.hpp"

using namespace cshr;
class NoTest {};

class Test : public AddListener<Test>,
             public std::enable_shared_from_this<Test> {
 private:
  using TestSharedPtr = std::shared_ptr<Test>;
  using CallBack = std::function<void(TestSharedPtr)>;

 public:
  bool addListenerOnSuccess(const CallBack &cb) override {
    {
      std::lock_guard<std::mutex> m(m_SuccessLock);
      m_Success.emplace_back(cb);
    }
    return true;
  }
  bool addListenerOnFail(const CallBack &cb) override {
    {
      std::lock_guard<std::mutex> m(m_FailLock);
      m_Fail.emplace_back(cb);
    }
    return true;
  }
  bool addListenerOnComplete(const CallBack &cb) override {
    {
      std::lock_guard<std::mutex> m(m_CompleteLock);
      m_Complete.emplace_back(cb);
    }
    return true;
  }
  void run() {
    std::vector<CallBack> pendingFunctors;
    {
        std::lock_guard<std::mutex> m(m_SuccessLock);
        pendingFunctors.swap(m_Success);
    }
    for(auto &cb: pendingFunctors) {
        cb(shared_from_this());
    }
  }

 private:
  std::vector<CallBack> m_Success, m_Fail, m_Complete;
  std::mutex m_SuccessLock, m_FailLock, m_CompleteLock;
};

int main() { 
    auto test = std::make_shared<Test>();
    // Observer<NoTest> obb;
    Observer<Test> ob;
    for(int i = 0; i < 10; ++i) {
        ob.bindTarget(test);
        ob.addListenerOnSuccess([=](std::shared_ptr<Test> ctx){
            std::cout << i << std::endl;
        });
    }
    test->run();
}