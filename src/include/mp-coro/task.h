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

#include <mp-coro/bits/task_promise_storage.h>
#include <mp-coro/coro_ptr.h>
#include <mp-coro/concepts.h>
#include <mp-coro/trace.h>
#include <mp-coro/type_traits.h>
#include <concepts>
#include <coroutine>

namespace mp_coro {

template<task_result T = void>
class [[nodiscard]] task {
public:
  using value_type = T;
  
  struct promise_type : detail::task_promise_storage<T> {
    std::coroutine_handle<> continuation = std::noop_coroutine();

    static std::suspend_always initial_suspend() noexcept
    { 
      TRACE_FUNC();
      return {};
    }

    static awaiter_of<void> auto final_suspend() noexcept
    {
      struct final_awaiter : std::suspend_always {
        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> this_coro) noexcept
        {
          TRACE_FUNC();
          return this_coro.promise().continuation;
        }
      };
      TRACE_FUNC();
      return final_awaiter{};
    }

    task<T> get_return_object() noexcept
    {
      TRACE_FUNC();
      return this;
    }
  };

  task(task&&) = default;
  task& operator=(task&&) = delete;

  awaiter_of<T> auto operator co_await() const noexcept
  {
    TRACE_FUNC();
    return awaiter(*promise_);
  }

  awaiter_of<const T&> auto operator co_await() const & noexcept
    requires std::move_constructible<T>
  {
    TRACE_FUNC();
    return awaiter(*promise_);
  }

  awaiter_of<T&&> auto operator co_await() const && noexcept
    requires std::move_constructible<T>
  {
    TRACE_FUNC();

    struct rvalue_awaiter : awaiter {
      T&& await_resume()
      {
        TRACE_FUNC();
        return std::move(this->promise).get();
      }
    };
    return rvalue_awaiter({*promise_});
  }

private:
  promise_ptr<promise_type> promise_;

  task(promise_type* promise): promise_(promise)
  {
    TRACE_FUNC();
  }

  struct awaiter {
    promise_type& promise;

    bool await_ready() const noexcept
    {
      TRACE_FUNC();
      return std::coroutine_handle<promise_type>::from_promise(promise).done();
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) const noexcept
    {
      TRACE_FUNC();
      promise.continuation = h;
      return std::coroutine_handle<promise_type>::from_promise(promise);
    }

    decltype(auto) await_resume() const
    {
      TRACE_FUNC();
      return this->promise.get();
    }
  };
};

template<awaitable A>
task<remove_rvalue_reference_t<await_result_t<A>>> make_task(A&& awaitable)
{
  TRACE_FUNC();
  co_return co_await std::forward<A>(awaitable);
}

} // namespace mp_coro
