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

#include <mp-coro/coro_ptr.h>
#include <mp-coro/trace.h>
#include <concepts>
#include <coroutine>
#include <optional>
#include <ranges>

namespace mp_coro {

template<typename T>
class [[nodiscard]] generator {
public:
  using value_type = std::remove_reference_t<T>;
  using reference = const value_type&;
  using pointer = const value_type*;

  class promise_type {
    pointer value_;
  public:
    static std::suspend_always initial_suspend() noexcept { TRACE_FUNC(); return {}; }
    static std::suspend_always final_suspend() noexcept { TRACE_FUNC(); return {}; }
    static void return_void() noexcept { TRACE_FUNC(); }

    generator<T> get_return_object() noexcept { TRACE_FUNC(); return this; }
    std::suspend_always yield_value(reference value) noexcept
    {
      TRACE_FUNC();
      value_ = std::addressof(value);
      return {};
    }
    void unhandled_exception() { TRACE_FUNC(); throw; }
    
    // disallow co_await in generator coroutines
    void await_transform() = delete;
    
    // custom functions
    [[nodiscard]] reference value() const noexcept { TRACE_FUNC(); return *value_; }
  };

  class const_iterator {
    std::coroutine_handle<promise_type> handle_;
  public:
    using value_type = generator::value_type;
    using difference_type = std::ptrdiff_t;
    
    const_iterator() = default;
    explicit const_iterator(promise_type& promise) noexcept : handle_(std::coroutine_handle<promise_type>::from_promise(promise)) {}

    const_iterator& operator++()
    {
      TRACE_FUNC();
      handle_.resume();
      return *this;
    }
    void operator++(int) { TRACE_FUNC(); static_cast<void>(operator++()); }

    [[nodiscard]] reference operator*() const noexcept { TRACE_FUNC(); return handle_.promise().value(); }
    [[nodiscard]] pointer operator->() const noexcept { TRACE_FUNC(); return std::addressof(operator*()); }

    [[nodiscard]] bool operator==(std::default_sentinel_t) const { TRACE_FUNC(); return !handle_ || handle_.done(); }
  };
  static_assert(std::input_iterator<const_iterator>);

  using iterator = const_iterator;

  [[nodiscard]] const_iterator begin()
  {
    TRACE_FUNC();
    auto handle = std::coroutine_handle<promise_type>::from_promise(*promise_);
    handle.resume();
    return const_iterator(*promise_);
  }
  [[nodiscard]] std::default_sentinel_t end() const noexcept { TRACE_FUNC(); return {}; }
private:
  promise_ptr<promise_type> promise_;
  generator(promise_type* promise): promise_(promise) {}
};

} // namespace mp_coro
