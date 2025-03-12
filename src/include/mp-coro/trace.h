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

#if !defined(MP_CORO_TRACE_LEVEL) || MP_CORO_TRACE_LEVEL == 0

#define TRACE_FUNC()

#else  // !defined(MP_CORO_TRACE_LEVEL) || MP_CORO_TRACE_LEVEL == 0

#include <iostream>
#include <source_location>
#include <syncstream>

namespace mp_coro {

namespace detail {

struct location {
  std::source_location data;
  friend std::ostream& operator<<(std::ostream& os, const location& loc)
  {
    os << loc.data.file_name() << " (" << loc.data.line() << ") `" << loc.data.function_name() << "`";
    return os;
  }
};

}  // namespace detail

#if MP_CORO_TRACE_LEVEL == 1

void trace_func(std::source_location loc = std::source_location::current())
{
  std::osyncstream(std::cout) << "[TRACE]: " << detail::location{loc} << '\n';
}

#define TRACE_FUNC() ::mp_coro::trace_func()

#elif MP_CORO_TRACE_LEVEL == 2

namespace detail {

struct [[nodiscard]] trace_on_finish {
  location loc;
  ~trace_on_finish() { std::osyncstream(std::cout) << "[TRACE]: " << loc << ": FINISH\n"; }
};

}  // namespace detail

detail::trace_on_finish trace_func(std::source_location loc = std::source_location::current())
{
  std::osyncstream(std::cout) << "[TRACE]: " << detail::location{loc} << ": START\n";
  return detail::trace_on_finish{loc};
}

#define TRACE_FUNC() auto _ = ::mp_coro::trace_func()

#endif /* MP_CORO_TRACE_LEVEL == 2 */

}  // namespace mp_coro

#endif  // !defined(MP_CORO_TRACE_LEVEL) || MP_CORO_TRACE_LEVEL == 0
