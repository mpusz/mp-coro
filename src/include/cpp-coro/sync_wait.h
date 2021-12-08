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

#include <cpp-coro/bits/get_awaiter.h>
#include <cpp-coro/bits/task_promise_storage.h>
#include <cpp-coro/coro_ptr.h>
#include <cpp-coro/trace.h>
#include <cpp-coro/type_traits.h>
#include <coroutine>
#include <semaphore>

namespace mp_coro {

namespace detail {

// ********* SYNC WAIT TASK *********

template<typename T = void>
  requires std::movable<T> || std::is_reference_v<T> || std::is_void_v<T>
class [[nodiscard]] sync_wait_task {
public:
  using value_type = T;
  
  struct promise_type : task_promise_storage<T> {
    std::binary_semaphore& signal;

    static std::suspend_never initial_suspend() noexcept
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
          return this_coro.promise().signal.release();
        }
      };
      TRACE_FUNC();
      return final_awaiter{};
    }

    promise_type(std::binary_semaphore& sync_signal, auto&&...): signal(sync_signal) {}

    sync_wait_task<T> get_return_object() noexcept
    {
      TRACE_FUNC();
      return this;
    }
  };

  // custom functions
  [[nodiscard]] decltype(auto) get() const &
  {
    TRACE_FUNC();
    return promise_->get();
  }

  [[nodiscard]] decltype(auto) get() const &&
  {
    TRACE_FUNC();
    return std::move(promise_)->get();
  }

private:
  promise_ptr<promise_type> promise_;

  sync_wait_task(promise_type* promise): promise_(promise)
  {
    TRACE_FUNC();
  }
};

// ********* SYNC WAIT CORO *********

template<awaitable A>
sync_wait_task<await_result_t<A>> sync_wait_coro(std::binary_semaphore&, A&& awaitable)
{ 
  TRACE_FUNC();
  co_return co_await std::forward<A>(awaitable);
}

} // namespace detail


// ********* SYNC WAIT *********

template<awaitable A>
[[nodiscard]] decltype(auto) sync_wait(A&& awaitable)
{
  TRACE_FUNC();
  std::binary_semaphore signal(0);
  auto coro = detail::sync_wait_coro(signal, std::forward<A>(awaitable));
  signal.acquire();
  return coro.get();
}

} // namespace mp_coro
