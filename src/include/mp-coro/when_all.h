// The MIT License (MIT)
//
// Copyright (c) 2021 Mateusz Pusz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <mp-coro/bits/synchronized_task.h>
#include <mp-coro/concepts.h>
#include <mp-coro/trace.h>
#include <mp-coro/type_traits.h>
#include <atomic>
#include <coroutine>
#include <cstdint>
#include <tuple>

namespace mp_coro {

namespace detail {

  template<typename T>
  static decltype(auto) make_when_all_result(T&& t)
  {
    return std::apply([&]<typename... Tasks>(Tasks&&... tasks) {
      using ret_type = std::tuple<remove_rvalue_reference_t<decltype(std::forward<Tasks>(tasks).nonvoid_get())>...>;
      return ret_type(std::forward<Tasks>(tasks).nonvoid_get()...);
    }, std::forward<T>(t));
  }

class when_all_sync {
  std::atomic<std::intmax_t> counter_;
  std::coroutine_handle<> continuation_;
public:
  constexpr when_all_sync(std::intmax_t count) noexcept
    : counter_(count + 1) // +1 for attaching a continuation
  {}
  when_all_sync(when_all_sync&& other) noexcept
    : counter_(other.counter_.load()), continuation_(other.continuation_)
  {}

  // Returns false when a continuation is being attached when all work is already done
  // and the current coroutine should be resumed right away via Symmetric Control Transfer.
  bool set_continuation(std::coroutine_handle<> cont)
  {
    continuation_ = cont;
    return counter_.fetch_sub(1, std::memory_order_acq_rel) > 1;
  }

  // On completion of each task a counter is being decremented and if the continuation is already
  // attached (a counter was decremented by `set_continuation()`) it is being resumed.
  void notify_awaitable_completed()
  {
    if(counter_.fetch_sub(1, std::memory_order_acq_rel) == 1)
      continuation_.resume();
  }

  // True if the continuation is already assigned which means that someone already awaited for
  // the awaitable completion.
  bool is_ready() const
  {
    return static_cast<bool>(continuation_);
  }
};

template<specialization_of<std::tuple> Tuple>
struct when_all_awaitable {
  explicit when_all_awaitable(Tuple&& tuple): tasks_(std::move(tuple)) {}

  bool await_ready() const noexcept { TRACE_FUNC(); return sync_.is_ready(); }
  bool await_suspend(std::coroutine_handle<> handle)
  {
    TRACE_FUNC();
    std::apply([&](auto&&... tasks){ (..., tasks.start(sync_)); }, tasks_);
    return sync_.set_continuation(handle);
  }
  decltype(auto) await_resume() &
  {
    TRACE_FUNC();
    return make_when_all_result(tasks_);
  }
  decltype(auto) await_resume() &&
  {
    TRACE_FUNC();
    return make_when_all_result(std::move(tasks_));
  }
private:
  Tuple tasks_;
  when_all_sync sync_ = std::tuple_size_v<Tuple>;
};

} // namespace detail

template<awaitable... Awaitables>
awaitable_of<std::tuple<remove_rvalue_reference_t<nonvoid_await_result_t<Awaitables>>...>> auto when_all(Awaitables&&... awaitables)
{
  TRACE_FUNC();
  return detail::when_all_awaitable(std::make_tuple(detail::make_synchronized_task<detail::when_all_sync>(std::forward<Awaitables>(awaitables))...));
}

} // namespace mp_coro
