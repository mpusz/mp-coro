#include <mp-coro/bits/cancellation_state.h>
#include <mp-coro/cancellation.h>

namespace mp_coro::detail {
bool cancellation_state::request_cancellation() noexcept
{  // If cancellation is already requested, then do nothing. Otherwise, we are the first to request cancellation.
  // NOTE: a thread that calls to request_cancellation and finds the requested_flag set, can try to help
  // with calling all the callbacks (just taking care to avoid calling fetch_add more than once).
  auto old_state = state_.fetch_or(cancellation_requested_flag);
  if (old_state & cancellation_requested_flag) return true;
  call_registrations();
  state_.fetch_add(cancellation_notification_complete_flag, std::memory_order_release);
  return false;
}

void cancellation_state::call_registrations()
{  // Traverse and call callbacks
  while (!registry_.empty())
    if (auto cr = registry_.try_deregister_one(); cr)  // C++23 `and_then` would be nice.
      cr.value()->callback_();  // nullptr dereference cannot happen in here (it would be a programming error).
}
}  // namespace mp_coro::detail
