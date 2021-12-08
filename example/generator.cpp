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

#include <cpp-coro/generator.h>
#include <cstdint>
#include <iostream>
#include <ranges>
#include <utility>
#include <vector>

mp_coro::generator<int> simple()
{
  co_yield 1;
  co_yield 2;
}

mp_coro::generator<std::uint64_t> fibonacci()
{
  std::uint64_t a = 0, b = 1;
  while (true) {
    co_yield b;
    a = std::exchange(b, a + b);
  }
}

template<std::integral T>
mp_coro::generator<T> range(T first, T last)
{
  while(first < last)
    co_yield first++;
}

mp_coro::generator<int&> ref()
{
  static std::vector data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  for(int& i : data)
    co_yield i;
}

mp_coro::generator<int> broken()
{
  co_yield 1;
  throw std::runtime_error("Some error\n");
  co_yield 2;
}

int main()
{
  try {
    for(auto v : simple())
      std::cout << v << ' ';
    std::cout << '\n';

    auto gen = fibonacci();
    for(auto i : std::views::counted(gen.begin(), 10))
      std::cout << i << ' ';
    std::cout << '\n';

    gen = fibonacci();
    for(const auto& i : gen | std::views::take_while([](auto i){ return i < 1000; }))
      std::cout << i << ' ';
    std::cout << '\n';

    for(auto i : range(65, 91))
      std::cout << static_cast<char>(i) << ' ';
    std::cout << '\n';

    for(auto& v : ref())
      std::cout << v << ' ';
    std::cout << '\n';

    for(auto v : broken())
      std::cout << v << ' ';
    std::cout << '\n';
  }
  catch(const std::exception& ex) {
    std::cout << "Unhandled exception: " << ex.what() << '\n';
  }
}
