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
#include <mp-coro/concepts.h>
#include <mp-coro/trace.h>
#include <exception>

namespace mp_coro {

namespace detail {

class eager_task {
public:
  struct promise_type : private detail::noncopyable {
    static std::suspend_never initial_suspend() noexcept { return {}; }
    static std::suspend_never final_suspend() noexcept { return {}; }
    static eager_task get_return_object() noexcept { return {}; }
    static void return_void() noexcept {}
    static void unhandled_exception() noexcept { std::terminate(); }
  };
private:
  eager_task() = default;
};

template<awaitable_of<void> A>
eager_task make_eager_task(A awaitable)
{
  co_return co_await std::move(awaitable);
}

}  // namespace detail

template<awaitable_of<void> A>
void spawn(A awaitable)
{
  TRACE_FUNC();
  detail::make_eager_task(std::move(awaitable));
}

}  // namespace mp_coro
