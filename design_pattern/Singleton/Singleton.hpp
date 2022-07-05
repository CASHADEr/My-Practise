#pragma once
#include <memory>

namespace cshr {
class Singleton {
 private:
  Singleton(/* args */);
  Singleton(const Singleton &) = delete;

 public:
  static Singleton &getInstance() {
    static Singleton instance;
    return instance;
  }
};

class SingletonDynamic {
 private:
  SingletonDynamic();
  SingletonDynamic(const SingletonDynamic &) = delete;

 public:
  static SingletonDynamic &getInstance() {
    static std::unique_ptr<SingletonDynamic> sptr{new SingletonDynamic()};
    return *sptr.get();
  }
};
}  // namespace cshr