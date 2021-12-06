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

#include <cpp-coro/task.h>
#include <cpp-coro/sync_wait.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <syncstream>
#include <thread>

struct tid_t {
  friend std::ostream& operator<<(std::ostream& os, tid_t)
  {
    return os << "(tid=" << std::this_thread::get_id() << ')';
  }
};
inline constexpr tid_t tid;

std::jthread worker;

mp_coro::task<std::string::size_type> async_read_file(const std::filesystem::path& path)
{
  struct awaitable {
    awaitable(std::filesystem::path path): path_(std::move(path)) {}
    static bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle)
    {
      auto work = [handle, this] {
        std::osyncstream(std::cout) << tid << " worker thread: opening file " << path_ << '\n';
        auto stream = std::ifstream(path_);
        std::osyncstream(std::cout) << tid << " worker thread: reading file\n";
        result_.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
        std::osyncstream(std::cout) << tid << " worker thread: resuming coro\n";
        handle.resume();
        std::osyncstream(std::cout) << tid << " worker thread: exiting\n";
      };
      worker = std::jthread(work);  // TODO: Fix that (replace with a thread pool)
    }
    std::string await_resume() noexcept { return std::move(result_); }
  private:
    const std::filesystem::path& path_;
    std::string result_;
  };

  std::osyncstream(std::cout) << tid << " readFile(): about to read file async\n";
  const std::string result = co_await awaitable(path);
  std::osyncstream(std::cout) << tid << " readFile(): about to return (size " << result.size() << ")\n";

  co_return result.size();
}

void f1(const std::filesystem::path& path)
{
  auto t = async_read_file(path);
  std::osyncstream(std::cout) << "Result: " << sync_wait(t) << '\n';
}

mp_coro::task<> f2(const std::filesystem::path& path)
{
  auto t = async_read_file(path);
  std::osyncstream(std::cout) << "Result: " << co_await t << '\n';
}

int main()
{
  try {
    auto path = "/etc/passwd";
    f1(path);
    sync_wait(f2(path));
  }
  catch(const std::exception& ex) {
    std::osyncstream(std::cout) << "Unhandled exception: " << ex.what() << '\n';
  }
}
