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
#include <mp-coro/concepts.h>
#include <mp-coro/generator.h>
#include <mp-coro/task.h>

using namespace mp_coro;

// std::suspend_always
static_assert(awaiter<std::suspend_always>);
static_assert(awaiter_of<std::suspend_always, void>);
static_assert(awaitable<std::suspend_always>);
static_assert(awaitable_of<std::suspend_always, void>);

// async<int(*)()>
static_assert(awaitable_of<async<int(*)()>, int&&>);
static_assert(awaitable_of<async<int(*)()>&&, int&&>);
static_assert(!awaitable<async<int(*)()>&>);
static_assert(!awaitable<const async<int(*)()>&>);

// async<int&(*)()>
static_assert(awaitable_of<async<int&(*)()>, int&>);
static_assert(awaitable_of<async<int&(*)()>&&, int&>);
static_assert(!awaitable<async<int&(*)()>&>);
static_assert(!awaitable<const async<int&(*)()>&>);

// async<const int&(*)()>
static_assert(awaitable_of<async<const int&(*)()>, const int&>);
static_assert(awaitable_of<async<const int&(*)()>&&, const int&>);
static_assert(!awaitable<async<const int&(*)()>&>);
static_assert(!awaitable<const async<const int&(*)()>&>);

// async<void(*)()>
static_assert(awaitable_of<async<void(*)()>, void>);
static_assert(awaitable_of<async<void(*)()>&&, void>);
static_assert(!awaitable<async<void(*)()>&>);
static_assert(!awaitable<const async<int(*)()>&>);

// task<int>
static_assert(awaitable_of<task<int>, int&&>);
static_assert(awaitable_of<task<int>&, const int&>);
static_assert(awaitable_of<const task<int>&, const int&>);
static_assert(awaitable_of<task<int>&&, int&&>);

// task<int&>
static_assert(awaitable_of<task<int&>, int&>);
static_assert(awaitable_of<task<int&>&, int&>);
static_assert(awaitable_of<const task<int&>&, int&>);
static_assert(awaitable_of<task<int&>&&, int&>);

// task<const int&>
static_assert(awaitable_of<task<const int&>, const int&>);
static_assert(awaitable_of<task<const int&>&, const int&>);
static_assert(awaitable_of<const task<const int&>&, const int&>);
static_assert(awaitable_of<task<const int&>&&, const int&>);

// task<void>
static_assert(awaitable_of<task<void>, void>);
static_assert(awaitable_of<task<void>&, void>);
static_assert(awaitable_of<const task<void>&, void>);
static_assert(awaitable_of<task<void>&&, void>);

// generator<int>
static_assert(!awaitable<generator<int>>);
static_assert(std::input_iterator<generator<int>::iterator>);
static_assert(std::ranges::input_range<generator<int>>);
static_assert(std::ranges::viewable_range<generator<int>>);
static_assert(std::ranges::view<generator<int>>);

int main()
{
}
