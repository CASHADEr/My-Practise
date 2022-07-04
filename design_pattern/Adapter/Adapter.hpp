#pragma once
#include <memory>
/**
 * 适配器模式: 面向对象
 * 将一个类的接口转换为客户想要的另一个接口，使得原本不能一起工作的类可以一起工作
 */

namespace cshr {
  // 目标接口
  class ITarget {
  public:
    virtual void execute() = 0;
    virtual ~ITarget() {}
  };

  // 原始接口
  class IAdaptee {
  public:
    virtual void oldExecute() = 0;
    virtual ~IAdaptee() {}
  };

  // 对象适配器
  // unique指针包裹的对象，适配之后生命期由Adapter支配
  class IAdapter : public ITarget {
  private:
    using AdapteePtr = IAdaptee *;
    using AdapteeUptr = std::unique_ptr<IAdaptee>;

  public:
    template<typename Target>
    IAdapter(std::unique_ptr<Target> &&adaptee)
        : m_adaptee(std::forward<std::unique_ptr<Target> &&>(adaptee)) {}
    template<typename Target>
    IAdapter(Target *adaptee) : m_adaptee(adaptee) {}
    void execute() override { m_adaptee->oldExecute(); }
    ~IAdapter() {}

  private:
    AdapteeUptr m_adaptee;
  };

  // 类适配器
  class Adapter : public ITarget, protected IAdaptee {
  public:
    void execute() override { oldExecute(); }
    ~Adapter() {}
  };
};  // namespace cshr
