#include <memory>
#include <utility>
namespace cshr {
struct TaskWrapperBase {
 public:
  virtual void call() = 0;
  void operator()() { this->call(); }
  virtual ~TaskWrapperBase() = default;
};
template <class Fn>
struct TaskWrapperImpl : public TaskWrapperBase {
  TaskWrapperImpl(std::enable_if_t<std::is_move_constructible_v<Fn>, Fn> &&fn)
      : mFn(std::move(fn)) {}
  TaskWrapperImpl(const TaskWrapperImpl &) = delete;
  TaskWrapperImpl &operator=(const TaskWrapperImpl &) = delete;
  void call() override { mFn(); }

 private:
  Fn mFn;
};
class TaskWrapper {
 public:
  TaskWrapper() = default;
  template <class Fn>
  TaskWrapper(Fn &&fn)
      : mFnPtr(std::make_unique<TaskWrapperImpl<std::decay_t<Fn>>>(
            std::forward<Fn>(fn))) {}
  TaskWrapper(TaskWrapper &&) = default;
  TaskWrapper &operator=(TaskWrapper &&) = default;
  void operator()() { (*mFnPtr)(); }

 private:
  std::unique_ptr<TaskWrapperBase> mFnPtr;
};
}  // namespace cshr