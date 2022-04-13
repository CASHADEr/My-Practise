#pragma once
#include <assert.h>

#include <atomic>
#include <memory>

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
  ConcurrentNodeT(value_uptr&& _data_ptr, ConcurrentNodeT* _next = nullptr)
      : data_ptr_(std::move(_data_ptr)), next_(_next) {}
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
  inline void modifyNext(ConcurrentNodeT* _next) { next_ = _next; }
  inline ConcurrentNodeT* next() { return next_; }

 public:
  value_uptr data_ptr_;
  ConcurrentNodeT* next_;
};

template <typename _Ty>
class StackMetaNode {
  friend class ConNodeTraits<StackMetaNode<_Ty>>;

 private:
  using node_ptr = ConcurrentNodeT<_Ty>*;
  using value_uptr = typename ConNodeTraits<ConcurrentNodeT<_Ty>>::value_uptr;
  using atomic_ptr = std::atomic<node_ptr>;

 public:
  StackMetaNode() = default;
  StackMetaNode(node_ptr _node_ptr) : node_ptr_(ATOMIC_VAR_INIT(_node_ptr)) {}

 public:
  atomic_ptr node_ptr_;
};

template <typename _Ty,
          typename _Sanitizer = HazardSanitizer<ConcurrentNodeT<_Ty>, 16>>
class LockFreeListStack {
  using MemorySanitizer = _Sanitizer;
  using node_type = ConcurrentNodeT<_Ty>;
  using node_ptr = node_type*;
  using stack_node_type = StackMetaNode<_Ty>;
  using atomic_node = std::atomic<stack_node_type>;
  using value_uptr = typename ConNodeTraits<node_type>::value_uptr;

 private:
 public:
  inline size_t size() { return size_.load(); }
  void Push(const _Ty& value) {
    node_ptr ori_node_ptr = head_.node_ptr_.load();
    node_ptr new_node_ptr = new node_type(value, ori_node_ptr);
    while (
        !head_.node_ptr_.compare_exchange_strong(ori_node_ptr, new_node_ptr)) {
      node_ptr ori_node_ptr = head_.node_ptr_.load();
      new_node_ptr->next_ = ori_node_ptr;
    }
    size_.fetch_add(1);
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
      if (head_.node_ptr_.compare_exchange_strong(new_node_ptr,
                                                  new_node_ptr->next_)) {
        ret.swap(ori_node_ptr->data_ptr_);
        size_.fetch_sub(1);
        break;
      }
    }
    sanitizer.reset();
    sanitizer.release(ori_node_ptr);
#ifdef __cshr_debug__
    __pop_count.fetch_add(1);
#endif
    return ret;
  }
#ifdef __cshr_debug__
 private:
  std::atomic<int> __pop_count{ATOMIC_VAR_INIT(0)};
#endif
 public:
  LockFreeListStack(bool openSanitizer = true)
      : head_(nullptr), size_(ATOMIC_VAR_INIT(0)), sanitizer{openSanitizer} {
    assert(head_.node_ptr_.is_lock_free());
  }
  ~LockFreeListStack() {
#ifdef __cshr_debug__
    cshrlog("Stack(%p) pop %d times.\n", this, __pop_count.load());
#endif
    cshrlog("Clean Stack(%p)...\n", this);
    while (Pop())
      ;
    cshrlog("Stack(%p) is clean...\n", this);
  }

 private:
  stack_node_type head_;
  std::atomic<size_t> size_;
  MemorySanitizer sanitizer{true};
};

template <typename _Ty>
class QueueNode {
  friend class ConNodeTraits<QueueNode<_Ty>>;

 private:
  using value_uptr = std::unique_ptr<_Ty>;
  using atomic_ptr = std::atomic<QueueNode<_Ty>*>;

 public:
  QueueNode() = default;
  QueueNode(const _Ty& _value, QueueNode* _next = nullptr)
      : data_ptr_(new _Ty(_value)), next_(ATOMIC_VAR_INIT(_next)) {}
  QueueNode(const value_uptr& _data_ptr) = delete;
  QueueNode(value_uptr&& _data_ptr) : data_ptr_(std::move(_data_ptr)) {}
  QueueNode& operator=(const QueueNode&) = delete;
  QueueNode(const QueueNode<_Ty>&) = delete;
  QueueNode(QueueNode<_Ty>&& _node)
      : data_ptr_(std::move(_node.data_ptr_)), next_(_node) {
    _node.next_.store(nullptr);
  }
  QueueNode& operator=(QueueNode&& _node) {
    if (&_node == this) return &this;
    data_ptr_ = std::move(_node.data_ptr_);
    next_ = _node.next_;
    _node.next_.store(nullptr);
  }
  ~QueueNode() {}

 public:
  inline void modifyNext(QueueNode* _next) { next_.store(_next); }
  inline QueueNode* next() { return next_.load(); }

 public:
  value_uptr data_ptr_;
  atomic_ptr next_;
};

template <typename _Ty>
class QueueMetaNode {
  friend class ConNodeTraits<QueueNode<_Ty>>;

 private:
  using node_ptr = QueueNode<_Ty>*;
  using value_uptr = typename ConNodeTraits<QueueNode<_Ty>>::value_uptr;
  using atomic_ptr = std::atomic<node_ptr>;

 public:
  QueueMetaNode() = default;
  QueueMetaNode(node_ptr _node_ptr) : node_ptr_(ATOMIC_VAR_INIT(_node_ptr)) {}
  QueueMetaNode(const QueueMetaNode& q)
      : node_ptr_(ATOMIC_VAR_INIT(q.node_ptr_.load())) {}

 public:
  atomic_ptr node_ptr_;
};

template <typename _Ty,
          typename _Sanitizer = HazardSanitizer<QueueNode<_Ty>, 16>>
class LockFreeListQueue {
  using MemorySanitizer = _Sanitizer;
  using node_type = QueueNode<_Ty>;
  using node_ptr = node_type*;
  using queue_node_type = QueueMetaNode<_Ty>;
  using atomic_node = std::atomic<queue_node_type>;
  using value_uptr = typename ConNodeTraits<node_type>::value_uptr;

 public:
  LockFreeListQueue(bool openSanitizer = true)
      : size_(ATOMIC_VAR_INIT(0)),
        head_(new node_type()),
        tail_(head_),
        sanitizer{openSanitizer} {
    assert(head_.node_ptr_.is_lock_free());
  }
  ~LockFreeListQueue() {
#ifdef __cshr_debug__
    cshrlog("Queue(%p) pop %d times.\n", this, __pop_count.load());
#endif
    cshrlog("Clean Queue(%p)...\n", this);
    while (Pop())
      ;
    delete head_.node_ptr_.load();
    cshrlog("Queue(%p) is clean...\n", this);
  }

 public:
  inline size_t size() { return size_.load(); }
  void Push(const _Ty& value) {
    node_ptr new_node_ptr = new node_type(value, nullptr);
    while (true) {
      node_ptr ori_node_ptr = tail_.node_ptr_.load();
      node_ptr next = ori_node_ptr->next_.load();

      // The tail_ is not the newest
      if (next) {
        tail_.node_ptr_.compare_exchange_strong(ori_node_ptr, next);
        continue;
      }

      // append new Node to tail_
      if (ori_node_ptr->next_.compare_exchange_strong(next, new_node_ptr)) {
        tail_.node_ptr_.compare_exchange_strong(ori_node_ptr, new_node_ptr);
        size_.fetch_add(1);
        break;
      }
    }
  }

  value_uptr Pop() {
    value_uptr ret;
    while (true) {
      node_ptr head_node_ptr = head_.node_ptr_.load();
      node_ptr tail_node_ptr = tail_.node_ptr_.load();
      node_ptr next = head_node_ptr->next_.load();
      // Empty
      if (head_node_ptr == tail_node_ptr && next == nullptr) {
        return ret;
      }
      // tail_ is behind, so reset the tail_
      if (head_node_ptr == tail_node_ptr && next != nullptr) {
        tail_.node_ptr_.compare_exchange_strong(tail_node_ptr, next);
        continue;
      }
      // Node available
      // first accessing the address
      // Note: The implementation includes two nodes per execution
      // Free the head pointer and get unique pointer from next pointer
      // So before the head_ is shifted, don't free the head pointer
      // Bbefore the data is retrieved, don't free the next pointer
      sanitizer.access(head_node_ptr);
      sanitizer.access(next);
      if (head_.node_ptr_.compare_exchange_strong(head_node_ptr, next)) {
        ret.swap(next->data_ptr_);
        size_.fetch_sub(1);
        sanitizer.reset();
        sanitizer.release(head_node_ptr);
#ifdef __cshr_debug__
        __pop_count.fetch_add(1);
#endif
        break;
      }
    }
    return ret;
  }
#ifdef __cshr_debug__
 private:
  std::atomic<int> __pop_count{ATOMIC_VAR_INIT(0)};
#endif

 private:
  std::atomic<size_t> size_;
  queue_node_type head_, tail_;
  MemorySanitizer sanitizer{true};
};
};  // namespace cshr