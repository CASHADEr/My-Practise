#include "refbase.hpp"

#include "../log/cshrlog.hpp"
namespace cshr {
RefBase::RefBase() : refcount_{ATOMIC_VAR_INIT(0)} {}
RefBase::~RefBase() {}
size_t inline RefBase::use_count() { return refcount_.load(); }
void RefBase::Increase() {
  cshrlog("counter: %d -> ", (int)use_count());
  refcount_.fetch_add(1);
  cshrlog("%d\n", (int)use_count());
}
void RefBase::Decrease() {
  cshrlog("counter: %d -> ", (int)use_count());
  if (__builtin_expect(refcount_.fetch_sub(1) == 1, 0)) {
    cshrlog("%d\n", (int)use_count());
    delete this;
  } else {
    cshrlog("%d\n", (int)use_count());
  }
}
}  // namespace cshr
