#include "refbase.hpp"

#include "../log/cshrlog.hpp"
namespace cshr {
RefBase::RefBase() {}
RefBase::~RefBase() {}
void RefBase::RefCountIncrease() { refcount_.fetch_add(1); }
void RefBase::RefCountDecrease() {
  if (refcount_.fetch_sub(1) == 1) {
    delete this;
  }
}
}  // namespace cshr
