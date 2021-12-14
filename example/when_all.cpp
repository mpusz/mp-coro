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
#include <mp-coro/when_all.h>
#include <mp-coro/task.h>
#include <iostream>
#include <syncstream>
#include <thread>

struct tid_t {
  friend std::ostream& operator<<(std::ostream& os, tid_t)
  {
    return os << "(tid=" << std::this_thread::get_id() << ')';
  }
};
inline constexpr tid_t tid;

template<typename Rep, typename Period>
void sleep_for(std::chrono::duration<Rep, Period> d)
{
  std::osyncstream(std::cout) << tid << ": about to sleep\n";
  std::this_thread::sleep_for(d);
  std::osyncstream(std::cout) << tid << ": about to return\n";
}

int main()
{
  using namespace mp_coro;
  using namespace std::chrono_literals;

  try {
    {
      awaitable_of<std::tuple<void_type, void_type, void_type>> auto work = when_all(async([]{ sleep_for(1s); }), async([]{ sleep_for(1s); }), async([]{ sleep_for(1s); }));
      auto result = sync_wait(work);
      static_assert(std::is_same_v<decltype(result), std::tuple<void_type, void_type, void_type>>);
    }

    {
      auto work = when_all(async([]{ return 1; }), async([]{ return 2; }), async([]{ return 3; }), async([]{ return 4; }));
      auto [v1, v2, v3, v4] = sync_await(work);
      std::cout << "v1: " << v1 << ", v2: " << v2 << ", v3: " << v3 << ", v4: " << v4 << '\n';
    }

    {
      auto [v1, v2, v3, v4] = sync_await(when_all(async([]{ return 5; }), async([]{ return 6; }), async([]{ return 7; }), async([]{ return 8; })));
      std::cout << "v1: " << v1 << ", v2: " << v2 << ", v3: " << v3 << ", v4: " << v4 << '\n';
    }
  }
  catch(const std::exception& ex) {
    std::cout << "Unhandled exception: " << ex.what() << '\n';
  }
}
