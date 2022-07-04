#include <functional>
#include <memory>

/**
 * 装饰器模式
 * 动态（组合）地给一个对象增加一些额外的职责。就增加功能而言，Decorator模式比生成子类（继承）更为灵活（消除重复代码
 *& 减少子类个数）
 *
 **/

namespace cshr {
class ITarget {
 public:
  virtual void execute() = 0;
  virtual ~ITarget() {}
};

class Decorator : public ITarget {
 private:
  using ITargetPtr = ITarget *;
  using ITargetSharedPtr = std::shared_ptr<ITarget>;
  using CallBack = std::function<void(const ITargetSharedPtr &)>;

 public:
  template <typename Target>
  Decorator(Target *target) : m_target(target) {}
  template <typename Target>
  Decorator(const std::shared_ptr<Target> &target) : m_target(target) {}
  void bindPreCall(const CallBack &cb) { m_PreCall = cb; }
  void bindPostCall(const CallBack &cb) { m_PostCall = cb; }
  void execute() override {
    if (m_PreCall) m_PreCall(m_target);
    m_target->execute();
    if (m_PostCall) m_PostCall(m_target);
  }
  ~Decorator() {}

 private:
  CallBack m_PreCall, m_PostCall;
  ITargetSharedPtr m_target;
};
};  // namespace cshr
