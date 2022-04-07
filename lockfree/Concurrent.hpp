#include <atomic>
#include <memory>
#include <assert.h>

#include "Sanitizer.hpp"
namespace cshr {
template <typename _ConNode>
struct ConNodeTraits {
  using value_uptr = typename _ConNode::value_uptr;
};
template <typename _Ty>
class ConcurrentNodeT {
  friend class ConNodeTraits<ConcurrentNodeT<_Ty>>;
 private:
  using value_uptr = std::unique_ptr<_Ty>;

 public:
  ConcurrentNodeT() = default;
  ConcurrentNodeT(const _Ty& _value, ConcurrentNodeT* _next = nullptr)
      : data_ptr_(new _Ty(_value)), next_(_next) {}
  ConcurrentNodeT(const value_uptr& _data_ptr,
                  ConcurrentNodeT* _next = nullptr) = delete;
  ConcurrentNodeT(const value_uptr&& _data_ptr,
                  ConcurrentNodeT* _next = nullptr)
      : data_ptr_(_data_ptr), next_(_next) {}
  ConcurrentNodeT& operator=(const ConcurrentNodeT&) = delete;
  ConcurrentNodeT(const ConcurrentNodeT<_Ty>&) = delete;
  ConcurrentNodeT(ConcurrentNodeT<_Ty>&& _node)
      : data_ptr_(std::move(_node.data_ptr_)), next_(_node) {
    _node.next_ = nullptr;
  }
  ConcurrentNodeT& operator=(ConcurrentNodeT&& _node) {
    if (&_node == this) return &this;
    data_ptr_ = std::move(_node.data_ptr_);
    next_ = _node.next_;
    _node.next_ = nullptr;
  }
  ~ConcurrentNodeT() {}

 public:
  value_uptr data_ptr_;
  ConcurrentNodeT* next_;
};

template <typename _Ty>
class StackNode {
  friend class ConNodeTraits<StackNode<_Ty>>;

 private:
  using node_ptr = ConcurrentNodeT<_Ty>*;
  using value_uptr = typename ConNodeTraits<ConcurrentNodeT<_Ty>>::value_uptr;
  using atomic_ptr = std::atomic<node_ptr>;

 public:
  bool operator==(const StackNode<_Ty>& target) {
    return this->node_ptr_ == target.node_ptr_ && this->stack_size_ == target.stack_size_;
  }
  bool operator!=(const StackNode<_Ty>& target) { return !(*this == target); }
  StackNode() = default;
  StackNode(node_ptr _node_ptr, size_t _size)
      : node_ptr_(ATOMIC_VAR_INIT(_node_ptr)), stack_size_(_size) {}

 public:
  atomic_ptr node_ptr_;
  size_t stack_size_;
};

template <typename _Ty,
          typename _Sanitizer = HazardSanitizer<ConcurrentNodeT<_Ty>, 16>>
class LockFreeStack {
//   using size_t = size_t;
  using MemorySanitizer = _Sanitizer;
  using node_type = ConcurrentNodeT<_Ty>;
  using node_ptr = node_type*;
  using stack_node_type = StackNode<_Ty>;
  using atomic_node = std::atomic<stack_node_type>;
  using value_uptr = typename ConNodeTraits<node_type>::value_uptr;

 private:
 public:
  inline size_t size() { return head_.load().stack_size_; }
  void Push(const _Ty& value) {
    node_ptr ori_node_ptr = head_.node_ptr_.load();
    node_ptr new_node_ptr = new node_type(value, ori_node_ptr);
    while (!head_.node_ptr_.compare_exchange_strong(ori_node_ptr, new_node_ptr)) {
      node_ptr ori_node_ptr = head_.node_ptr_.load();
      new_node_ptr->next_ = ori_node_ptr;
    }
  }
  value_uptr Pop() {
    value_uptr ret;
    auto ori_node_ptr = head_.node_ptr_.load();
    while (true) {
      if (!ori_node_ptr) return ret;
      // accessing
      sanitizer.access(ori_node_ptr);
      // recheck the validation
      auto new_node_ptr = head_.node_ptr_.load();
      if (ori_node_ptr != new_node_ptr) {
        ori_node_ptr = new_node_ptr;
        continue;
      }
      if (head_.node_ptr_.compare_exchange_strong(new_node_ptr, new_node_ptr->next_)) {
        ret.swap(ori_node_ptr->data_ptr_);
        break;
      }
    }
    sanitizer.deaccess();
    sanitizer.release(ori_node_ptr);
    return ret;
  }
  LockFreeStack() : head_(nullptr, 0) {
    assert(head_.node_ptr_.is_lock_free());
  }
  ~LockFreeStack() {}

 private:
  stack_node_type head_;
  MemorySanitizer sanitizer{true};
};
};  // namespace cshr