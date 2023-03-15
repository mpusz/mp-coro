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

#include <mp-coro/async.h>
#include <mp-coro/sync_await.h>
#include <mp-coro/task.h>
#include <concepts>
#include <iostream>
#include <optional>
#include <syncstream>

mp_coro::task<int> foo()
{
  const int res = co_await mp_coro::async([] { return 42; });
  co_await mp_coro::async([&] { std::cout << "Result: " << res << std::endl; });
  co_return res + 23;
}

mp_coro::task<> bar()
{
  const auto res = co_await foo();
  std::cout << "Result of foo: " << res << std::endl;
}

mp_coro::task<int> boo()
{
  const int res = co_await mp_coro::async([] {
    std::cout << "About to throw an exception\n";
    throw std::runtime_error("Some error");
    std::cout << "This will never be printed\n";
    return 42;
  });
  std::cout << "I will never tell you that the result is: " << res << std::endl;
  co_return res;
}

mp_coro::task<> example()
{
  co_await bar();
  co_await boo();
}

template<typename T>
void test(mp_coro::task<T> t)
{
  try {
    if constexpr (std::is_void_v<T>)
      sync_await(t);
    else
      std::cout << "Result: " << sync_await(t) << '\n';
  } catch (const std::exception& ex) {
    std::cout << "Exception caught: " << ex.what() << "\n";
  }
}

int main()
{
  try {
    test(example());
    test(boo());
    test(mp_coro::make_task(mp_coro::async([] { return 123; })));
  } catch (const std::exception& ex) {
    std::cout << "Unhandled exception: " << ex.what() << "\n";
  }
}
