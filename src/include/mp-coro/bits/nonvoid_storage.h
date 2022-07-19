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
#include <mp-coro/type_traits.h>

namespace mp_coro {

struct void_type {};

template<typename T>
using nonvoid_await_result_t = std::conditional_t<std::is_void_v<await_result_t<T>>, void_type, await_result_t<T>>;

namespace detail {

template<typename T>
class nonvoid_storage : public storage<T> {
public:
  using value_type = typename storage<T>::value_type;

  [[nodiscard]] const value_type& nonvoid_get() const & { return this->get(); }
  [[nodiscard]] value_type&& nonvoid_get() && { return std::move(*this).get(); }

  [[nodiscard]] T& nonvoid_get() const & requires std::is_reference_v<T> { return this->get(); }
};

template<>
class nonvoid_storage<void> : public storage<void> {
public:
  using value_type = void_type;

  [[nodiscard]] void_type nonvoid_get() const
  {
    check_and_rethrow(this->result);
    return void_type{};
  }
};

} // namespace detail
} // namespace mp_coro
