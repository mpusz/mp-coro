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
#include <chrono>
#include <iostream>
#include <thread>

mp_coro::task<int> async_foo()
{
  struct awaitable {
    int result;
    static bool await_ready() noexcept
    {
      TRACE_FUNC();
      return false;
    }
    void await_suspend(std::coroutine_handle<> handle)
    {
      auto work = [&, handle] {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(200ms);
        result = 42;
        handle.resume();
      };
      TRACE_FUNC();
      std::jthread(work).detach();  // TODO: Fix that (replace with a thread pool)
    }
    int await_resume() noexcept
    {
      TRACE_FUNC();
      return result;
    }
  };
  co_return co_await awaitable{};
}

mp_coro::task<long> bar()
{
  const auto task = async_foo();
  const long i = co_await task;
  co_return i + 23;
}

int main()
{
  try {
    std::cout << sync_await(bar()) << "\n";
  } catch (const std::exception& ex) {
    std::cout << "Unhandled exception caught: " << ex.what() << "\n";
  }
}
