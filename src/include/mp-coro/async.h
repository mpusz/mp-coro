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

#include <mp-coro/bits/storage.h>
#include <mp-coro/trace.h>
#include <concepts>
#include <coroutine>
#include <thread>

namespace mp_coro {

template<std::invocable Func>
class async {
public:
  using return_type = std::invoke_result_t<Func>;

  template<std::convertible_to<Func> F>
  explicit async(F&& func) : func_{std::forward<F>(func)}
  {
  }

  decltype(auto) operator co_await() & = delete;  // async should be co_awaited only once (on rvalue)
  decltype(auto) operator co_await() &&
  {
    struct awaiter {
      async& awaitable;
      bool await_ready() const noexcept
      {
        TRACE_FUNC();
        return false;
      }
      void await_suspend(std::coroutine_handle<> handle)
      {
        auto work = [&, handle]() {
          TRACE_FUNC();
          try {
            if constexpr (std::is_void_v<return_type>)
              awaitable.func_();
            else
              awaitable.result_.set_value(awaitable.func_());
          } catch (...) {
            awaitable.result_.set_exception(std::current_exception());
          }
          handle.resume();
        };

        TRACE_FUNC();
        std::jthread(work).detach();  // TODO: Fix that (replace with a thread pool)
      }
      decltype(auto) await_resume()
      {
        TRACE_FUNC();
        return std::move(awaitable.result_).get();
      }
    };
    return awaiter{*this};
  }
private:
  Func func_;
  detail::storage<return_type> result_;
};

template<std::invocable F>
async(F) -> async<F>;

}  // namespace mp_coro
