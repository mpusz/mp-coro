#pragma once

#include <mp-coro/bits/cancellation_state.h>
#include <mp-coro/bits/noncopyable.h>
#include <concepts>
#include <functional>
#include <memory>
#include <utility>

namespace mp_coro {

struct operation_cancelled : std::exception {
  operation_cancelled() {}
  const char* what() const noexcept override { return "Operation cancelled"; }
};

class cancellation_token {
public:
  cancellation_token(detail::cancellation_state* s) : state_{s} {}

  [[nodiscard]] bool can_be_cancelled() const noexcept { return state_.get() != nullptr && state_->can_be_cancelled(); }

  [[nodiscard]] bool is_cancellation_requested() const noexcept
  {
    return state_.get() != nullptr && state_->is_cancellation_requested();
  }

  // operations specific to cancellation_token

  void throw_if_cancellation_requested()
  {
    if (state_.get() != nullptr && is_cancellation_requested()) throw operation_cancelled{};
  }
private:
  detail::token_cancellation_state_ptr state_;
};

class cancellation_source {
public:
  cancellation_source() : state_{detail::cancellation_state::make_cancellation_state()} {}

  [[nodiscard]] bool can_be_cancelled() const noexcept { return state_.get() != nullptr && state_->can_be_cancelled(); }

  [[nodiscard]] bool is_cancellation_requested() const noexcept
  {
    return state_.get() != nullptr && state_->is_cancellation_requested();
  };

  // operations specific to cancellation_source

  void request_cancellation()
  {
    if (state_.get() != nullptr) state_->request_cancellation();
  }

  cancellation_token token() const noexcept { return cancellation_token(state_.get()); }

private:
  detail::source_cancellation_state_ptr state_;
};

}  // namespace mp_coro

// TODO
// - factor out common parts of `cancellation_token` and `cancellation_source` into `cancellation_base`.
//   - main problem: do not virtualize the destructor.
//   - lesser problem: constructors also differ.
//   - idea: in lisp this could be a macro generating the class when given the customized function.
//     But inheritance isn't quite like that so probably don't CRTP (and C macros are error-prone).
// - should implementations be moved to a .cpp file?
