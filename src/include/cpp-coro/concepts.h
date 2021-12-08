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
#include <cpp-coro/bits/type_traits.h>
#include <concepts>
#include <coroutine>

namespace mp_coro {

namespace detail {

template<typename Ret, typename Handle>
Handle func_arg(Ret (*)(Handle));

template<typename Ret, typename T, typename Handle>
Handle func_arg(Ret (T::*)(Handle));

template<typename Ret, typename T, typename Handle>
Handle func_arg(Ret (T::*)(Handle) &);

template<typename Ret, typename T, typename Handle>
Handle func_arg(Ret (T::*)(Handle) &&);

template<typename Ret, typename T, typename Handle>
Handle func_arg(Ret (T::*)(Handle) const);

template<typename Ret, typename T, typename Handle>
Handle func_arg(Ret (T::*)(Handle) const &);

template<typename Ret, typename T, typename Handle>
Handle func_arg(Ret (T::*)(Handle) const &&);

template<typename T>
concept suspend_return_type =
  std::is_void_v<T> ||
  std::is_same_v<T, bool> ||
  specialization_of<T, std::coroutine_handle>;

} // namespace detail

template<typename T>
concept awaiter = requires(T&& t) {
  { std::forward<T>(t).await_ready() } -> std::convertible_to<bool>;
  { detail::func_arg(&std::remove_reference_t<T>::await_suspend) } -> std::convertible_to<std::coroutine_handle<>>; // TODO Why gcc does not inherit from `std::coroutine_handle<>`?
  { std::forward<T>(t).await_suspend(std::declval<decltype(detail::func_arg(&std::remove_reference_t<T>::await_suspend))>()) } -> detail::suspend_return_type;
  std::forward<T>(t).await_resume();
};

template<typename T, typename Value>
concept awaiter_of = awaiter<T> && requires(T&& t) {
  { std::forward<T>(t).await_resume() } -> std::same_as<Value>;
};

template<typename T>
concept awaitable = requires(T&& t) {
  { detail::get_awaiter(std::forward<T>(t)) } -> awaiter;
};

template<typename T, typename Value>
concept awaitable_of = awaitable<T> && requires(T&& t) {
  { detail::get_awaiter(std::forward<T>(t)) } -> awaiter_of<Value>;
};

} // namespace mp_coro
