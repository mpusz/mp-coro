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

#include <coroutine>
#include <utility>

namespace mp_coro {

template<typename Promise>
class coro_handle {
public:
  using handle_type = std::coroutine_handle<Promise>;

  explicit coro_handle(handle_type h) : h_(h) {}
  explicit coro_handle(Promise& p) : h_(handle_type::from_promise(p)) {}

  coro_handle(coro_handle&& other) noexcept : h_(std::exchange(other.h_, nullptr)) {}
  coro_handle& operator=(coro_handle&& other) noexcept
  {
    if (h_) h_.destroy();
    h_ = std::exchange(other.h_, nullptr);
    return *this;
  }
  coro_handle(const coro_handle&) = delete;
  coro_handle& operator=(const coro_handle&) = delete;
  ~coro_handle()
  {
    if (h_) h_.destroy();
  }

  explicit operator bool() const noexcept { return static_cast<bool>(h_); }
  const handle_type* operator->() const noexcept { return &h_; }
  const handle_type& operator*() const noexcept { return h_; }
private:
  handle_type h_;
};

}  // namespace mp_coro
