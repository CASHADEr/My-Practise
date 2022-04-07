#include <atomic>
#include <chrono>
#include <thread>

namespace cshr {
template <typename _Deriv, typename _Node>
class _MemorySanitizer {
 protected:
  using node_type = _Node;
  using node_ptr = node_type*;

 public:
  inline _Deriv& selfCast() { return static_cast<_Deriv&>(*this); }
  inline bool access(node_ptr _node_ptr) { selfCast().access(_node_ptr); }
  inline bool release(node_ptr _node_ptr) { selfCast().release(_node_ptr); }
  inline bool deaccess() { return access(nullptr); }
  _MemorySanitizer() {}
  virtual ~_MemorySanitizer() {}
};

template <typename _Node, size_t _ThreadMax>
class HazardSanitizer
    : public _MemorySanitizer<HazardSanitizer<_Node, _ThreadMax>, _Node> {
  // using size_t = size_t;
  using _Base = _MemorySanitizer<HazardSanitizer<_Node, _ThreadMax>, _Node>;
  using node_type = typename _Base::node_type;
  using node_ptr = typename _Base::node_ptr;
  using atomic_node_ptr = std::atomic<node_ptr>;
  using id_type = std::thread::id;
  using atomic_id = std::atomic<id_type>;
  class HazardPointer {
   public:
    inline id_type id() { return id_.load(); }
    inline node_ptr accessing() { return accessing_ptr_.load(); }

   public:
    atomic_id id_;
    atomic_node_ptr accessing_ptr_;
  };
  using access_array = HazardPointer[_ThreadMax];

 private:
  inline HazardPointer* _RegisterThread(const id_type& id) {
    id_type init_id;
    for (size_t i = 0; i < _ThreadMax; ++i) {
      if (hps_[i].id_.compare_exchange_strong(init_id, id)) {
        return &hps_[i];
      }
    }
    return nullptr;
  }
  inline HazardPointer* _FindThreadAccess(const id_type& id) {
    for (size_t i = 0; i < _ThreadMax; ++i) {
      if (hps_[i].id() == id) {
        return &hps_[i];
      }
    }
    return nullptr;
  }
  inline void _ImmediateDelete(node_ptr _node_ptr) { delete _node_ptr; }
  inline void _DelayDelete(node_ptr _node_ptr) {
    do {
      _node_ptr->next_ = backward_deleting_list_.load();
    } while (!backward_deleting_list_.compare_exchange_strong(_node_ptr->next_,
                                                              _node_ptr));
  }
  inline void _Delete(node_ptr _node_ptr) {
    size_t i = 0;
    while (i < _ThreadMax && hps_[i].accessing() != _node_ptr) {
      ++i;
    }
    if (i == _ThreadMax) {
      _ImmediateDelete(_node_ptr);
    } else {
      _DelayDelete(_node_ptr);
    }
  }
  void _BackLoopDelete() {
    node_ptr to_delete = backward_deleting_list_.exchange(nullptr);
    while (to_delete) {
      node_ptr _node = to_delete;
      to_delete = to_delete->next_;
      _Delete(_node);
    }
  }
  static void BackSanitizerThread(HazardSanitizer<_Node, _ThreadMax>*,
                                  std::chrono::milliseconds);

 private:
  class HazardRegister {
   public:
    HazardRegister(HazardSanitizer<_Node, _ThreadMax>* context)
        : hp(context->_RegisterThread(std::this_thread::get_id())) {}
    ~HazardRegister() {
      if (!hp) return;
      hp->id_.store(std::thread::id());
      hp->accessing_ptr_.store(nullptr);
    }
    HazardPointer* hp;
  };

  inline bool _Store(node_ptr _node_ptr) {
    thread_local HazardRegister cur_register(this);
    if (!_node_ptr || !cur_register.hp) return false;
    cur_register.hp->accessing_ptr_.store(_node_ptr);
    return true;
  }
  inline bool _Release(node_ptr _node_ptr) noexcept {
    if (!_node_ptr) return false;
    _Delete(_node_ptr);
    if (!isBackSanitizer_) _BackLoopDelete();  // 可以放到一个背景清理线程
    return true;
  }

 public:
  HazardSanitizer(bool isBackSanitizer = false)
      : isBackSanitizer_(isBackSanitizer) {
    using namespace std::chrono_literals;
    if (isBackSanitizer) {
      backSanitizer_ = std::thread(BackSanitizerThread, this, 2000ms);
    }
  }
  ~HazardSanitizer() {
    if (isBackSanitizer_) {
      isBackSanitizer_.store(false);
      backSanitizer_.join();
    }
    node_ptr to_delete = backward_deleting_list_.exchange(nullptr);
    while (to_delete) {
      node_ptr _node_ptr = to_delete;
      to_delete = to_delete->next_;
      _ImmediateDelete(_node_ptr);
    }
  }
  bool access(node_ptr _node_ptr) { return _Store(_node_ptr); }
  bool release(node_ptr _node_ptr) { return _Release(_node_ptr); }

 private:
  access_array hps_ = {};
  atomic_node_ptr backward_deleting_list_ = ATOMIC_VAR_INIT(nullptr);
  std::atomic<bool> isBackSanitizer_;
  std::thread backSanitizer_;
};
template <typename _Node, size_t _ThreadMax>
void HazardSanitizer<_Node, _ThreadMax>::BackSanitizerThread(
    HazardSanitizer<_Node, _ThreadMax>* context, std::chrono::milliseconds ms) {
  while (context->isBackSanitizer_.load()) {
    context->_BackLoopDelete();
    std::this_thread::sleep_for(ms);
  }
}
};  // namespace cshr