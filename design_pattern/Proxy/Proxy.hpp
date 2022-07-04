#include <memory>
#include <string>

namespace cshr {
class ITarget {
 public:
  virtual void execute() = 0;
  virtual ~ITarget(){}
};
class Target: public ITarget {
 public:
   void execute() override;
   ~Target();
};
class Proxy : public ITarget {
 private:
  using TargetPtr = ITarget *;
  using TargetSharedPtr = std::shared_ptr<ITarget>;

 public:
  template <typename Target>
  Proxy(Target *target) : m_target(target), m_IsAdmin(false) {}
  template <typename Target>
  Proxy(const std::shared_ptr<Target> &target) : m_target(target), m_IsAdmin(false) {}
  bool login(const std::string &username, const std::string &password) {
    m_user = username;
    if(m_user == "cshr") m_IsAdmin = true;
    return true;
  }
  void execute() override;
  ~Proxy();
 private:
  TargetSharedPtr m_target;
  std::string m_user;
  bool m_IsAdmin;
};
};  // namespace cshr