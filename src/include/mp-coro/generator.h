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
#include <mp-coro/bits/noncopyable.h>
#include <mp-coro/trace.h>
#include <cassert>
#include <concepts>
#include <coroutine>
#include <optional>
#include <ranges>
#include <utility>

namespace mp_coro {

template<typename T>
class [[nodiscard]] generator {
public:
  using value_type = std::remove_reference_t<T>;
  using reference = std::conditional_t<std::is_reference_v<T>, T, const value_type&>;
  using pointer = std::add_pointer_t<reference>;

  struct promise_type : private detail::noncopyable {
    pointer value;

    static std::suspend_always initial_suspend() noexcept { TRACE_FUNC(); return {}; }
    static std::suspend_always final_suspend() noexcept { TRACE_FUNC(); return {}; }
    static void return_void() noexcept { TRACE_FUNC(); }

    generator get_return_object() noexcept { TRACE_FUNC(); return this; }
    std::suspend_always yield_value(reference v) noexcept
    {
      TRACE_FUNC();
      value = std::addressof(v);
      return {};
    }
    void unhandled_exception() { TRACE_FUNC(); throw; }
    
    // disallow co_await in generator coroutines
    void await_transform() = delete;
  };

  class iterator {
    std::coroutine_handle<promise_type> handle_;
    friend generator;
    explicit iterator(std::coroutine_handle<promise_type> h) noexcept : handle_(h) {}
  public:
    using value_type = generator::value_type;
    using difference_type = std::ptrdiff_t;
    
    iterator() = default;  // TODO Remove when gcc is fixed
    
    iterator(iterator&& other) noexcept: handle_(std::exchange(other.handle_, {})) {}
    iterator& operator=(iterator&& other) noexcept
    {
      handle_ = std::exchange(other.handle_, {});
      return *this;
    }

    iterator& operator++()
    {
      TRACE_FUNC();
      assert(!handle_.done() && "Can't increment generator end iterator");
      handle_.resume();
      return *this;
    }
    void operator++(int) { TRACE_FUNC(); ++*this; }

    [[nodiscard]] reference operator*() const noexcept
    {
      TRACE_FUNC();
      assert(!handle_.done() && "Can't dereference generator end iterator");
      return *handle_.promise().value;
    }
    [[nodiscard]] pointer operator->() const noexcept
    {
      TRACE_FUNC();
      return std::addressof(operator*());
    }

    [[nodiscard]] bool operator==(std::default_sentinel_t) const noexcept
    {
      TRACE_FUNC();
      return
        !handle_ || // TODO Remove when gcc is fixed (default-construction will not be available and user should not compare with a moved-from iterator)
        handle_.done();
    }
  };
  static_assert(std::input_iterator<iterator>);

  generator() = default;  // TODO Remove when gcc is fixed

  [[nodiscard]] iterator begin()
  {
    TRACE_FUNC();
    // Pre: Coroutine is suspended at its initial suspend point
    assert(promise_ && "Can't call begin on moved-from generator");
    auto handle = std::coroutine_handle<promise_type>::from_promise(*promise_);
    handle.resume();
    return iterator(handle);
  }
  [[nodiscard]] std::default_sentinel_t end() const noexcept { TRACE_FUNC(); return std::default_sentinel; }
private:
  promise_ptr<promise_type> promise_;
  generator(promise_type* promise): promise_(promise) {}
};

} // namespace mp_coro

template<typename T>
inline constexpr bool std::ranges::enable_view<mp_coro::generator<T>> = true;
