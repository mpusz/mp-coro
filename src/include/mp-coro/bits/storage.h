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

#include <exception>
#include <type_traits>
#include <variant>

namespace mp_coro::detail {

template<typename... Args>
void check_and_rethrow(const std::variant<Args...>& result)
{
  if (std::holds_alternative<std::exception_ptr>(result))
    std::rethrow_exception(std::get<std::exception_ptr>(std::move(result)));
}

template<typename T>
class storage_base {
protected:
  std::variant<std::monostate, std::exception_ptr, T> result;
public:
  template<std::convertible_to<T> U>
  void set_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, decltype(std::forward<U>(value))>)
  {
    result.template emplace<T>(std::forward<U>(value));
  }

  [[nodiscard]] const T& get() const&
  {
    check_and_rethrow(this->result);
    return std::get<T>(this->result);
  }

  [[nodiscard]] T&& get() &&
  {
    check_and_rethrow(this->result);
    return std::get<T>(std::move(this->result));
  }
};

template<typename T>
class storage_base<T&> {
protected:
  std::variant<std::monostate, std::exception_ptr, T*> result;
public:
  void set_value(T& value) noexcept { result = std::addressof(value); }

  [[nodiscard]] T& get() const
  {
    check_and_rethrow(this->result);
    return *std::get<T*>(this->result);
  }
};

template<>
class storage_base<void> {
protected:
  std::variant<std::monostate, std::exception_ptr> result;
public:
  void get() const { check_and_rethrow(this->result); }
};

template<typename T>
class storage : public storage_base<T> {
public:
  using value_type = T;
  void set_exception(std::exception_ptr ptr) noexcept { this->result = std::move(ptr); }
};

}  // namespace mp_coro::detail
