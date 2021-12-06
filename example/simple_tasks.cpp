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

#include <cpp-coro/sync_wait.h>
#include <cpp-coro/task.h>
#include <iostream>

mp_coro::task<int> foo()
{
  std::cout << "foo(): about to return\n";
  co_return 42;
}

mp_coro::task<int> bar()
{
  auto result = foo();
  std::cout << "bar(): about to co_await\n";
  const int i = co_await result;
  std::cout << "i = " << i << '\n';
  std::cout << "bar(): about to return\n" << std::endl;
  co_return i + 23;
}

int val = 123;

mp_coro::task<int&> ref()
{
  co_return val;
}

mp_coro::task<> baz()
{
  std::cout << co_await foo() << '\n';
  std::cout << co_await ref() << '\n';
}

mp_coro::task<> empty()
{
  std::cout << "empty\n";
  co_return;
}

int main()
{
  try {
    std::cout << "i = " << sync_wait(bar()) << '\n';
    sync_wait(baz());
    sync_wait(empty());
    std::cout << sync_wait(ref()) << '\n';
  }
  catch(const std::exception& ex) {
    std::cout << "Unhandled exception: " << ex.what() << '\n';
  }
}
