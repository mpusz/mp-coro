#pragma once

#include <mp-coro/bits/noncopyable.h>
#include <atomic>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace mp_coro {

// Forward declaration of cancellation_registration.
struct cancellation_registration;

namespace detail {
class [[nodiscard]] cancellation_state : noncopyable {
public:
  static cancellation_state* make_cancellation_state() { return new cancellation_state(); };

  void add_source_ref() { add_ref(cancellation_source_rc_pos); }

  void dec_source_ref() { dec_ref(cancellation_source_rc_pos); }

  void add_token_ref() { add_ref(cancellation_token_rc_pos); }

  void dec_token_ref() { dec_ref(cancellation_token_rc_pos); }


  bool request_cancellation() noexcept
  {  // Returns previous values of cancellation_requested.
    auto old_state = state_.fetch_or(cancellation_requested_flag);
    if (old_state & cancellation_requested_flag) return true;  // if cancellation is already requested
    // otherwise, we are the first to request cancellation
    state_.fetch_add(cancellation_notification_complete_flag, std::memory_order_release);
    return false;
  };


  [[nodiscard]] bool can_be_cancelled() const noexcept
  {
    return state_.load(std::memory_order_acquire) & can_be_cancelled_mask;
  }


  [[nodiscard]] bool is_cancellation_requested() const noexcept
  {
    return state_.load(std::memory_order_acquire) & cancellation_requested_flag;
  }

  [[nodiscard]] bool try_register_callback([[maybe_unused]] cancellation_registration* cancellation_registration)
  {
    if (is_cancellation_requested()) return false;
    // TODO: implement this using cancellation_register
    return false;
  }

private:
  ~cancellation_state() = default;

  void dec_ref(std::uint64_t cancellation_rc_pos)
  {
    // If old reference count was one, then the last reference got removed. Delete this object.
    auto old_state = state_.fetch_sub(cancellation_rc_pos, std::memory_order_acq_rel);
    if ((old_state & clean_rc_mask) == cancellation_rc_pos) delete this;
  }

  void add_ref(std::uint64_t cancellation_rc_pos)
  {
    // RMW relaxed operation forms part of the release sequence
    // between fetch_sub operations with acq_rel memory order.
    state_.fetch_add(cancellation_rc_pos, std::memory_order_relaxed);
  }

  // Initialized with source reference count set to 1.
  std::atomic<std::uint64_t> state_ = cancellation_source_rc_pos;

  // State and its default initialized value is a bit packed version of the following struct:
  // struct {
  //   bool cancellation_requested: 1 = false;                   <-- bit 0
  //   bool cancellation_notification_complete: 1 = false;       <-- bit 1
  //   std::uint32_t source_ref_count: 31 = 1;                   <-- bits 2 to 32
  //   std::uint32_t token_ref_count: 31 = 0;                    <-- bits 33 to 63
  // } state_;

  // Constants used to handle state:
  // - `cancellation_requested_flag`: indicates that a cancellation request has already been issued.
  // - `cancellation_notification_complete_flag`: Indicates that all tokens have been notified of cancellation and all
  // callbacks have been called.
  // - `cancellation_source_rc_pos` and `cancellation_token_rc_pos`: Bit positions of the reference counting for
  // cancellation_source and cancellation_token/cancellation_registration. clean_rc_mask: Mask used to dispose bits that
  // do not correspond to reference counting.
  // - `can_be_cancelled`: Mask used to indicate if cancellation_state is cancellable. State is cancellable if either
  // source refcount > 1, token/register refcount > 0, cancellation has been requested, or cancellation completion has
  // already been notified.
  static constexpr std::uint64_t cancellation_requested_flag = UINT64_C(1);
  static constexpr std::uint64_t cancellation_notification_complete_flag = UINT64_C(1) << 1;
  static constexpr std::uint64_t cancellation_source_rc_pos = UINT64_C(1) << 2;
  static constexpr std::uint64_t cancellation_token_rc_pos = UINT64_C(33) << 33;
  static constexpr std::uint64_t can_be_cancelled_mask = cancellation_token_rc_pos - 1;
  static constexpr std::uint64_t clean_rc_mask =
    ~(cancellation_requested_flag | cancellation_notification_complete_flag);
};


template<typename Derived>
struct cancellation_state_ptr_base {
  // RAII Pointer types to keep reference counting of sources and tokens (std::shared_ptr is not useful for this because
  // the reference counting cannot be specialized).
  cancellation_state_ptr_base() = default;
  cancellation_state_ptr_base(cancellation_state* s) : state_{s} { add_ref(); }
  ~cancellation_state_ptr_base() { dec_ref(); }

  cancellation_state_ptr_base(const cancellation_state_ptr_base& rhs) : state_{rhs.state_} { add_ref(); }
  cancellation_state_ptr_base& operator=(const cancellation_state_ptr_base& rhs)
  {
    state_ = rhs.state_;
    add_ref();
    return *this;
  }

  cancellation_state_ptr_base(cancellation_state_ptr_base&& rhs) : state_{std::exchange(rhs.state_, nullptr)} {}
  cancellation_state_ptr_base& operator=(cancellation_state_ptr_base&& rhs)
  {
    state_ = std::exchange(rhs.state_, nullptr);
    return *this;
  }

  cancellation_state& operator*() const { return *state_; }
  cancellation_state* operator->() const { return get(); }
  cancellation_state* get() const { return state_; }
  void reset(cancellation_state* new_ptr)
  {
    dec_ref();
    state_ = new_ptr;
    add_ref();
  }

private:
  void add_ref()
  {
    if (state_ != nullptr) static_cast<Derived*>(this)->add_ref();
  }

  void dec_ref()
  {
    if (state_ != nullptr) static_cast<Derived*>(this)->dec_ref();
  }

protected:
  cancellation_state* state_ = nullptr;
};

struct token_cancellation_state_ptr final : cancellation_state_ptr_base<token_cancellation_state_ptr> {
  void add_ref() { state_->add_token_ref(); }
  void dec_ref() { state_->dec_token_ref(); }
};

struct source_cancellation_state_ptr final : cancellation_state_ptr_base<source_cancellation_state_ptr> {
  void add_ref() { state_->add_token_ref(); }
  void dec_ref() { state_->dec_token_ref(); }
};

}  // namespace detail
}  // namespace mp_coro


// TODO when callback registration is implemented:
// - `try_register_callback` must be implemented.
// - `request_cancellation` should invoke callbacks.
// - `~cancellation_state` should free resources kept to hold registered callbacks
