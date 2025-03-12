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

#include <mp-coro/bits/synchronized_task.h>
#include <mp-coro/concepts.h>
#include <mp-coro/trace.h>
#include <mp-coro/type_traits.h>
#include <atomic>
#include <coroutine>
#include <cstdint>
#include <ranges>
#include <tuple>
#include <vector>

namespace mp_coro {

namespace detail {

class when_all_sync {
  std::atomic<std::size_t> counter_;
  std::coroutine_handle<> continuation_;
public:
  constexpr when_all_sync(std::size_t count) noexcept : counter_(count + 1)  // +1 for attaching a continuation
  {
  }
  when_all_sync(when_all_sync&& other) noexcept : counter_(other.counter_.load()), continuation_(other.continuation_) {}

  // Returns false when a continuation is being attached when all work is already done
  // and the current coroutine should be resumed right away via Symmetric Control Transfer.
  bool set_continuation(std::coroutine_handle<> cont)
  {
    continuation_ = cont;
    return counter_.fetch_sub(1, std::memory_order_acq_rel) > 1;
  }

  // On completion of each task a counter is being decremented and if the continuation is already
  // attached (a counter was decremented by `set_continuation()`) it is being resumed.
  void notify_awaitable_completed()
  {
    if (counter_.fetch_sub(1, std::memory_order_acq_rel) == 1) continuation_.resume();
  }

  // True if the continuation is already assigned which means that someone already awaited for
  // the awaitable completion.
  bool is_ready() const { return static_cast<bool>(continuation_); }
};


template<typename T>
std::size_t tasks_size(T& container)
{
  if constexpr (std::ranges::range<T>)
    return size(container);
  else
    return std::tuple_size_v<T>;
}

template<typename T>
decltype(auto) start_all_tasks(T& container, when_all_sync& sync)
{
  if constexpr (std::ranges::range<T>)
    for (auto& t : container) t.start(sync);
  else
    std::apply([&](auto&... tasks) { (..., tasks.start(sync)); }, container);
}

template<typename T>
decltype(auto) make_all_results(T&& container)
{
  if constexpr (std::ranges::range<T>) {
    if constexpr (std::is_void_v<typename std::ranges::range_value_t<T>::value_type>) {
      // in case of `void` check for exception and do not return any result
      for (auto& task : container) task.get();
    } else {
      std::vector<typename std::ranges::range_value_t<T>::value_type> result;
      result.reserve(size(container));
      for (auto&& task : std::forward<T>(container)) result.emplace_back(std::forward<decltype(task)>(task).get());
      return result;
    }
  } else {
    return std::apply(
      [&]<typename... Tasks>(Tasks&&... tasks) {
        if constexpr ((... && std::is_void_v<typename std::remove_cvref_t<Tasks>::value_type>)) {
          // in case of all `void` check for exception and do not return any result
          (..., tasks.get());
        } else {
          using ret_type = std::tuple<remove_rvalue_reference_t<decltype(std::forward<Tasks>(tasks).get())>...>;
          return ret_type(std::forward<Tasks>(tasks).get()...);
        }
      },
      std::forward<T>(container));
  }
}

template<typename T>
struct when_all_awaitable {
  explicit when_all_awaitable(T&& tasks) : tasks_(std::move(tasks)) {}

  decltype(auto) operator co_await() &
  {
    struct awaiter : awaiter_base {
      decltype(auto) await_resume()
      {
        TRACE_FUNC();
        return make_all_results(this->awaitable.tasks_);
      }
    };
    return awaiter{{*this}};
  }

  decltype(auto) operator co_await() &&
  {
    struct awaiter : awaiter_base {
      decltype(auto) await_resume()
      {
        TRACE_FUNC();
        return make_all_results(std::move(this->awaitable.tasks_));
      }
    };
    return awaiter{{*this}};
  }

private:
  struct awaiter_base {
    when_all_awaitable& awaitable;

    bool await_ready() const noexcept
    {
      TRACE_FUNC();
      return awaitable.sync_.is_ready();
    }
    bool await_suspend(std::coroutine_handle<> handle)
    {
      TRACE_FUNC();
      start_all_tasks(awaitable.tasks_, awaitable.sync_);
      return awaitable.sync_.set_continuation(handle);
    }
  };
  T tasks_;
  when_all_sync sync_ = tasks_size(tasks_);
};

}  // namespace detail

template<awaitable... Awaitables>
awaitable auto when_all(Awaitables&&... awaitables)
{
  TRACE_FUNC();
  return detail::when_all_awaitable(
    std::make_tuple(detail::make_synchronized_task<detail::when_all_sync>(std::forward<Awaitables>(awaitables))...));
}

template<std::ranges::range R>
awaitable auto when_all(R&& awaitables)
{
  TRACE_FUNC();
  std::vector<detail::synchronized_task<detail::when_all_sync, await_result_t<std::ranges::range_reference_t<R>>>>
    tasks;
  tasks.reserve(size(awaitables));
  for (auto&& awaitable : std::forward<R>(awaitables))
    tasks.emplace_back(
      detail::make_synchronized_task<detail::when_all_sync>(std::forward<decltype(awaitable)>(awaitable)));
  return detail::when_all_awaitable(std::move(tasks));
}

}  // namespace mp_coro
