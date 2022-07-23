#include <memory>
namespace cshr {
enum RbTreeColor { Rb_RED = false, Rb_BLACK = true };
// Rb Tree basic properties
// which contain color indicator and three pointers
struct RbTreeNodeBase {
  using BasePtr = RbTreeNodeBase *;
  using ConstBasePtr = const RbTreeNodeBase *;

 public:
  static BasePtr leftMost(BasePtr base_ptr) noexcept {
    while (base_ptr->M_left) base_ptr = base_ptr->M_left;
    return base_ptr;
  }
  static ConstBasePtr leftMost(ConstBasePtr base_ptr) noexcept {
    while (base_ptr->M_left) base_ptr = base_ptr->M_left;
    return base_ptr;
  }
  static BasePtr rightMost(BasePtr base_ptr) noexcept {
    while (base_ptr->M_right) base_ptr = base_ptr->M_right;
    return base_ptr;
  }
  static ConstBasePtr rightMost(ConstBasePtr base_ptr) noexcept {
    while (base_ptr->M_right) base_ptr = base_ptr->M_right;
    return base_ptr;
  }

 public:
  RbTreeColor M_color;
  BasePtr M_parent, M_left, M_right;
};

struct RbTreeHeader {
  RbTreeNodeBase M_header;
  size_t M_node_count;
  RbTreeHeader() noexcept
      : M_header{Rb_RED, nullptr, &M_header, &M_header}, M_node_count{0} {}
  void resetHeader() {
    M_header = {Rb_RED, nullptr, &M_header, &M_header};
    M_node_count = 0;
  }
};
// The RB tree node template
// which inherits RbTreeNodeBase and contains a value spot
template <typename Val>
struct RbNode : public RbTreeNodeBase {
 public:
  using value_type = Val;
  value_type *M_valptr() { return &value_field; }
  const value_type *M_valptr() const { return &value_field; }

 public:
  value_type value_field;
  RbNode *left, *right, *parent;
};

// The RB tree template
// which regulates the RB tree nodes
template <typename Key, typename Value, typename KeyOfValue, typename Compare,
          typename Alloc = std::allocator<Value>>
class RbTree {
  using RbNodeBase = RbNode<Value>;
  using NodeAlloc =
      typename __gnu_cxx::__alloc_traits<Alloc>::rebind<RbNode<Value>>::other;

  // The implementation of RbTree Template
  // which aggregates all the members
  // and can do the compress for Compare and NodeAlloc
  struct RbTreeImpl : public Compare, public NodeAlloc, public RbTreeHeader {
    using KeyCompare = Compare;
    RbTreeImpl() noexcept(
        std::is_nothrow_default_constructible<_Node_allocator>::value &&
            std::is_nothrow_default_constructible<_Base_key_compare>::value) {}
    RbTreeImpl(const RbTreeImpl &) = delete;
    RbTreeImpl(RbTreeImpl &&) = default;
  };

 protected:
  using BasePtr = RbNodeBase *;
  using ConstBasePtr = const RbNodeBase *;
  using NodePtr = RbNode<Value> *;
  using ConstNodePtr = const RbNode<Value> *;

 protected:
  BasePtr root() noexcept { return M_impl.M_header.M_parent; }
  ConstBasePtr root() const noexcept { return M_impl.M_header.M_parent; }
  BasePtr leftMost() noexcept { return M_impl.M_header.M_left; }
  ConstBasePtr leftMost() const noexcept { return M_impl.M_header.M_left; }
  BasePtr rightMost() noexcept { return M_impl.M_header.M_right; }
  ConstBasePtr rightMost() const noexcept { return M_impl.M_header.M_right; }
  BasePtr end() noexcept { return &M_impl.M_header; }
  ConstBasePtr end() const noexcept { return &M_impl.M_header; }

 protected:
  static const Key &keyof(ConstNodePtr x) {
    return KeyOfValue()(*x->M_valptr());
  }

 private:
  RbTreeImpl M_impl;
};
}  // namespace cshr
