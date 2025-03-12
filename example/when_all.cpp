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
#include <mp-coro/when_all.h>
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

struct lifetime {
  const char* txt = "active (default-constructed)";

  lifetime() = default;
  lifetime(const lifetime&) { txt = "active (copy-constructed)"; }
  lifetime& operator=(const lifetime&)
  {
    txt = "active (copy-assigned)";
    return *this;
  }

  lifetime(lifetime&& other) noexcept
  {
    txt = "active (move-constructed)", other.txt = "unspecified (move-constructed-from)";
  }
  lifetime& operator=(lifetime&& other) noexcept
  {
    txt = "active (move-assigned)";
    other.txt = "unspecified (move-assigned-from)";
    return *this;
  }

  ~lifetime() { txt = "destructed"; }

  void operator()() const { std::cout << "[lifetime] " << txt << '\n'; }
};

int main()
{
  using namespace mp_coro;
  using namespace std::chrono_literals;

  try {
    {
      auto func = [obj = lifetime{}] { obj(); };
      auto work = when_all(async(func), async(func), async(func));
      sync_await(work);
    }

    {
      auto func = [obj = lifetime{}] { obj(); };
      sync_await(when_all(async(func), async(func), async(func)));
    }

    {
      auto a1 = async([obj = lifetime{}] { obj(); });
      auto a2 = async([obj = lifetime{}] { obj(); });
      auto a3 = async([obj = lifetime{}] { obj(); });
      sync_await(when_all(std::move(a1), std::move(a2), std::move(a3)));
    }

    {
      auto work =
        when_all(async([] { return 1; }), async([] { return 2; }), async([] { return 3; }), async([] { return 4; }));
      auto [v1, v2, v3, v4] = sync_await(work);
      std::cout << "v1: " << v1 << ", v2: " << v2 << ", v3: " << v3 << ", v4: " << v4 << '\n';
    }

    {
      auto [v1, v2, v3, v4] = sync_await(
        when_all(async([] { return 5; }), async([] { return 6; }), async([] { return 7; }), async([] { return 8; })));
      std::cout << "v1: " << v1 << ", v2: " << v2 << ", v3: " << v3 << ", v4: " << v4 << '\n';
    }

    {
      std::vector<task<void>> v;
      for (int i = 0; i < 3; ++i) v.push_back(make_task(async([] { sleep_for(1s); })));
      sync_await(when_all(std::move(v)));
    }

    {
      std::vector<task<void>> v;
      for (int i = 0; i < 3; ++i) v.push_back(make_task(async([] { sleep_for(1s); })));
      auto a = when_all(v);
      sync_await(a);
    }
  } catch (const std::exception& ex) {
    std::cout << "Unhandled exception: " << ex.what() << '\n';
  }
}
