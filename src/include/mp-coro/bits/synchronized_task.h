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

#include <mp-coro/bits/noncopyable.h>
#include <mp-coro/bits/task_promise_storage.h>
#include <mp-coro/coro_handle.h>
#include <mp-coro/trace.h>
#include <mp-coro/type_traits.h>
#include <coroutine>

namespace mp_coro::detail {

template<typename Sync, typename T>
  requires requires(Sync s) { s.notify_awaitable_completed(); }
class [[nodiscard]] synchronized_task {
public:
  using value_type = T;

  struct promise_type : private detail::noncopyable, task_promise_storage<T> {
    Sync* sync = nullptr;

    static std::suspend_always initial_suspend() noexcept
    {
      TRACE_FUNC();
      return {};
    }

    static awaiter_of<void> auto final_suspend() noexcept
    {
      struct final_awaiter : std::suspend_always {
        void await_suspend(std::coroutine_handle<promise_type> this_coro) noexcept
        {
          TRACE_FUNC();
          this_coro.promise().sync->notify_awaitable_completed();
        }
      };
      TRACE_FUNC();
      return final_awaiter{};
    }

    synchronized_task get_return_object() noexcept
    {
      TRACE_FUNC();
      return synchronized_task{*this};
    }
  };

  // custom functions
  void start(Sync& s)
  {
    handle_->promise().sync = &s;
    handle_->resume();
  }

  [[nodiscard]] decltype(auto) get() const&
  {
    TRACE_FUNC();
    return handle_->promise().get();
  }

  [[nodiscard]] decltype(auto) get() const&&
  {
    TRACE_FUNC();
    return std::move(handle_->promise()).get();
  }

private:
  coro_handle<promise_type> handle_;
  explicit synchronized_task(promise_type& promise) : handle_(promise) { TRACE_FUNC(); }
};

template<typename Sync, awaitable A>
  requires requires(Sync s) { s.notify_awaitable_completed(); }
synchronized_task<Sync, remove_rvalue_reference_t<await_result_t<A>>> make_synchronized_task(A&& awaitable)
{
  TRACE_FUNC();
  co_return co_await std::forward<A>(awaitable);
}

}  // namespace mp_coro::detail
