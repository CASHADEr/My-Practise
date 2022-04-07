#pragma once
#include <stddef.h>

#include <atomic>
#include <iostream>

#include "../log/cshrlog.hpp"
namespace cshr {
class RefBase {
 public:
  size_t use_count() { return refcount_.load(); }
  void RefCountIncrease();
  void RefCountDecrease();

 public:
  RefBase();
  virtual ~RefBase() = 0;

 private:
  std::atomic<size_t> refcount_{0};
};
template <typename _Ty>
class sptr {
  using value_type = _Ty;
  using value_ptr = _Ty*;

 public:
  sptr(): object_ptr_(nullptr) {
  }
  sptr(const sptr& _sptrobj) : object_ptr_(_sptrobj.GetRefPtr()) {
    RefCountIncrease();
  }
  template <typename _Tx>
  sptr(const sptr<_Tx>& _sptrobj) : object_ptr_(_sptrobj.GetRefPtr()) {
    RefCountIncrease();
  }
  template <typename _Tx>
  sptr(_Tx _obj_ptr) : object_ptr_(_obj_ptr) {
    RefCountIncrease();
  }
  ~sptr() { RefCountDecrease(); }
  operator bool() { return object_ptr_ != nullptr; }

 private:
  void RefCountIncrease() {
    cshrlog("Increase\n");
    if (object_ptr_) object_ptr_->RefCountIncrease();
  }
  void RefCountDecrease() {
    cshrlog("Decrease\n");
    if (object_ptr_) object_ptr_->RefCountDecrease();
    object_ptr_ = nullptr;
  }

 public:
  sptr<value_type> operator=(value_ptr _obj_ptr) {
    RefCountDecrease();
    object_ptr_ = _obj_ptr;
    RefCountIncrease();
  }
  template <typename _Tx>
  sptr<_Ty> operator=(sptr<_Tx> _sptrobj) {
    RefCountDecrease();
    object_ptr_ = _sptrobj.GetRefPtr();
    RefCountIncrease();
  }
  value_ptr operator->() { return GetRefPtr(); }
  value_ptr GetRefPtr() const { return object_ptr_; }

 private:
  value_ptr object_ptr_ = nullptr;
};
template <typename _Ty>
class wptr {};
}  // namespace cshr
