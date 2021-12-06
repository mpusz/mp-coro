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

#include <cpp-coro/type_traits.h>
#include <concepts>

namespace mp_coro {

namespace detail {

template<typename T>
concept suspend_return_type_ =
  std::is_void_v<T> ||
  std::is_same_v<T, bool> ||
  specialization_of<T, std::coroutine_handle>;

} // namespace detail

template<typename T, typename U>
concept awaiter_of = requires(T&& t) {
  { std::forward<T>(t).await_ready() } -> std::convertible_to<bool>;
  { std::forward<T>(t).await_suspend(std::coroutine_handle<>{}) } -> detail::suspend_return_type_;
  { std::forward<T>(t).await_resume() } -> std::same_as<U>;
};

} // namespace mp_coro
