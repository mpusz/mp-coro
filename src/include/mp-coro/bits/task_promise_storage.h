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

namespace mp_coro::detail {

template<typename T>
struct task_promise_storage_base : storage<T> {
  void unhandled_exception() noexcept(noexcept(this->set_exception(std::current_exception())))
  {
    TRACE_FUNC();
    this->set_exception(std::current_exception());
  }
};

template<typename T>
struct task_promise_storage : task_promise_storage_base<T> {
  template<typename U>
  void return_value(U&& value) noexcept(noexcept(this->set_value(std::forward<U>(value))))
    requires requires { this->set_value(std::forward<U>(value)); }
  {
    TRACE_FUNC();
    this->set_value(std::forward<U>(value));
  }
};

template<>
struct task_promise_storage<void> : task_promise_storage_base<void> {
  void return_void() noexcept { TRACE_FUNC(); }
};

}  // namespace mp_coro::detail
