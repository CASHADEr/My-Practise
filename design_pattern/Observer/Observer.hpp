#pragma once
#include <functional>
#include <memory>
#include <type_traits>

/**
 * 观察者模式: 模板实现
 * 当一个对象状态改变时，所有依赖于它的对象都得到通知并且自动更新
 **/

namespace cshr {
template <typename Target>
class AddListener {
 public:
  virtual bool addListenerOnSuccess(
      const std::function<void(std::shared_ptr<Target>)> &) = 0;
  virtual bool addListenerOnFail(
      const std::function<void(std::shared_ptr<Target>)> &) = 0;
  virtual bool addListenerOnComplete(
      const std::function<void(std::shared_ptr<Target>)> &) = 0;
  virtual ~AddListener() {}
};

// Observer
// 绑定一个目标对象，按需向其注册回调函数
template <typename Target>
#if __cpluscplus >= 202002L
requires(std::is_base_of<AddListener<Target>, Target>::value)
#endif
class Observer : public AddListener<Target> {
 private:
  constexpr static bool _is_base_of_addlistener_target = std::is_base_of<AddListener<Target>, Target>::value;
  using TargetPtr = Target *;
  using TargetWeakPtr = std::weak_ptr<Target>;
  using TargetSharedPtr = std::shared_ptr<Target>;
  using CallBack = std::function<void(TargetSharedPtr)>;

 public:
  Observer() {
    static_assert(_is_base_of_addlistener_target);
  }
  template <typename _Derived>
  void bindTarget(const std::shared_ptr<_Derived> &target) {
    m_target = target;
  }
  template <typename _Derived>
  void bindTarget(const std::weak_ptr<_Derived> &target) {
    m_target = target;
  }
  bool isAlive() {
    return m_target.lock();
  }  // true是没有意义的，只能在false采取相应的操作
  bool addListenerOnSuccess(const CallBack &cb) override {
    auto ptr = m_target.lock();
    return ptr ? ptr->addListenerOnSuccess(cb) : false;
  }
  bool addListenerOnFail(const CallBack &cb) override {
    auto ptr = m_target.lock();
    return ptr ? ptr->addListenerOnFail(cb) : false;
  }
  bool addListenerOnComplete(const CallBack &cb) override {
    auto ptr = m_target.lock();
    return ptr ? ptr->addListenerOnComplete(cb) : false;
  }

 private:
  TargetWeakPtr m_target;
};
};  // namespace cshr