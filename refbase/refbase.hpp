#pragma once
#include <stddef.h>

#include <atomic>
#include <iostream>

#include "../log/cshrlog.hpp"
namespace cshr {
class RefBase {
 public:
  size_t use_count();
  void Increase();
  void Decrease();

 public:
  RefBase();
  virtual ~RefBase() = 0;

 private:
  std::atomic<size_t> refcount_;
};
template <typename _Ty>
class sptr {
  using value_type = _Ty;
  using value_ptr = _Ty*;

 public:
  sptr() noexcept : object_ptr_(nullptr) {
    cshrlog("sptr %p default initialized\n", this);
  }
  sptr(const sptr& _sptrobj) : object_ptr_(_sptrobj.GetRefPtr()) {
    cshrlog("sptr %p copy constructor from %p\n", this, &_sptrobj);
    RefCountIncrease();
  }
  template <typename _Tx>
  sptr(const sptr<_Tx>& _sptrobj) : object_ptr_(_sptrobj.GetRefPtr()) {
    cshrlog("sptr %p pointer constructor.\n", this);
    RefCountIncrease();
  }
  template <typename _Tx>
  sptr(sptr<_Tx>&& _sptrobj) : object_ptr_(_sptrobj.GetRefPtr()) {
    cshrlog("sptr %p move constructor from %p\n", this, &_sptrobj);
    RefCountIncrease();
    _sptrobj.release();
  }
  template <typename _Tx>
  sptr(_Tx _obj_ptr) : object_ptr_(_obj_ptr) {
    cshrlog("sptr %p raw pointer constructor.\n", this);
    RefCountIncrease();
  }
  ~sptr() {
    cshrlog("sptr %p destruct.\n", this);
    RefCountDecrease();
  }
  operator bool() { return object_ptr_ != nullptr; }

 private:
  void RefCountIncrease() {
    if (object_ptr_) {
      cshrlog("Increase pointer target: %p\n", object_ptr_);
      object_ptr_->Increase();
    }
  }
  void RefCountDecrease() {
    if (object_ptr_) {
      cshrlog("Decrease pointer target: %p\n", object_ptr_);
      object_ptr_->Decrease();
    }
    object_ptr_ = nullptr;
  }

 public:
  template <typename _Tx>
  sptr<value_type>& operator=(_Tx _obj_ptr) {
    cshrlog("sptr %p raw pointer assigned.\n", this);
    RefCountDecrease();
    object_ptr_ = _obj_ptr;
    cshrlog("assigned address: %p\n", object_ptr_);
    RefCountIncrease();
    return *this;
  }
  template <typename _Tx>
  sptr<value_type>& operator=(const sptr<_Tx>& _sptrobj) {
    cshrlog("sptr %p pointer copy assigned.\n", this);
    if (this == &_sptrobj) {
      return *this;
    }
    RefCountDecrease();
    object_ptr_ = _sptrobj.GetRefPtr();
    RefCountIncrease();
    return *this;
  }
  template <typename _Tx>
  sptr<value_type>& operator=(sptr<_Tx>&& _sptrobj) {
    cshrlog("sptr %p pointer move assigned.\n", this);
    if (this == &_sptrobj) {
      return *this;
    }
    RefCountDecrease();
    object_ptr_ = _sptrobj.GetRefPtr();
    RefCountIncrease();
    _sptrobj.release();
    return *this;
  }
  value_ptr operator->() { return GetRefPtr(); }
  value_ptr GetRefPtr() const { return object_ptr_; }
  value_ptr release() {
    RefCountDecrease();
    auto p = object_ptr_;
    object_ptr_ = nullptr;
    return p;
  }

 private:
  value_ptr object_ptr_ = nullptr;
};
template <typename _Ty>
class wptr {};
}  // namespace cshr
