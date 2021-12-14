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

#include <mp-coro/sync_await.h>
#include <mp-coro/task.h>
#include <mp-coro/type_traits.h>
#include <chrono>
#include <iostream>
#include <thread>

template<mp_coro::specialization_of<std::chrono::duration> D>
struct sleep_for {
  D duration;
  bool await_ready() const noexcept { return duration <= D::zero(); }
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> coro) const
  {
    std::this_thread::sleep_for(duration);
    return coro;
  }
  static void await_resume() noexcept {}
};

mp_coro::task<> sleepy()
{
  using namespace std::chrono_literals;
  std::cout << "sleepy(): about to sleep\n";
  co_await sleep_for{1s};
  std::cout << "sleepy(): about to return\n";
}

int main()
{
  try {
    mp_coro::sync_await(sleepy());
  }
  catch(const std::exception& ex) {
    std::cout << "Unhandled exception: " << ex.what() << '\n';
  }
}
