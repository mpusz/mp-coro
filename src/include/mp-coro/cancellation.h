#pragma once

#include <mp-coro/bits/cancellation_state.h>
#include <mp-coro/bits/noncopyable.h>
#include <cstdint>
#include <concepts>
#include <functional>
#include <memory>
#include <utility>

namespace mp_coro {

struct operation_cancelled : std::exception {
  operation_cancelled() {}
  const char* what() const noexcept override { return "Operation cancelled"; }
};


namespace detail {

template<typename CancellationStateSmartPtr>
class cancellation_common_base {
public:
  [[nodiscard]] bool can_be_cancelled() const noexcept { return state_.get() != nullptr && state_->can_be_cancelled(); }

  [[nodiscard]] bool is_cancellation_requested() const noexcept
  {
    return state_.get() != nullptr && state_->is_cancellation_requested();
  }

protected:
  cancellation_common_base(cancellation_state* s) : state_{s} {};
  CancellationStateSmartPtr state_;
};

}  // namespace detail


struct cancellation_token final : detail::cancellation_common_base<detail::token_cancellation_state_ptr> {
  cancellation_token(detail::cancellation_state* s) : cancellation_common_base{s} {}

  void throw_if_cancellation_requested()
  {
    if (state_.get() != nullptr && is_cancellation_requested()) throw operation_cancelled{};
  }

  friend struct cancellation_registration;
};


struct cancellation_source final : detail::cancellation_common_base<detail::source_cancellation_state_ptr> {
  cancellation_source() : cancellation_common_base{detail::cancellation_state::make_cancellation_state()} {}

  void request_cancellation()
  {
    if (state_.get() != nullptr) state_->request_cancellation();
  }

  cancellation_token token() const noexcept { return cancellation_token(state_.get()); }
};


struct cancellation_registration final : detail::noncopyable {
  template<typename Callback>
    requires std::constructible_from<Callback, std::function<void(void)>>
  cancellation_registration(cancellation_token token, Callback&& cb) : callback_{std::forward<Callback>(cb)}
  {  // taking token by value increases token_ref
    state_ = std::move(token.state_);
    if (state_.get() != nullptr && state_->can_be_cancelled() && !state_->try_register_callback(this)) {
      state_.reset(nullptr);  // cb failed to be registered, decrease token_ref
      callback_();
    }
  }

private:
  friend struct detail::cancellation_registry;
  friend class detail::cancellation_state;

  std::size_t bin_idx;
  std::function<void(void)> callback_;
  detail::token_cancellation_state_ptr state_;
};

}  // namespace mp_coro
