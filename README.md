[![GitHub license](https://img.shields.io/github/license/mpusz/mp-coro?cacheSeconds=3600&color=informational&label=License)](./LICENSE.md)

# `mp-coro` - A C++20 coroutine support library

The library originated as a support code for the ["Introduction to C++20"](https://train-it.eu/trainings/cpp/78-cpp-20)
workshop. The workshop covers the design choices and implementation steps needed to implement most of
the features provided here.

On the way it turned out that `cppcoro` is not maintained anymore and there are no good libraries of similar
quality (at least I was not able to find them). As a result this library was refactored and cleaned up
in the hope that others will find it useful and that the C++ community can provide feedback on it and ways
to improve it.

Still, one of the main goals of the library is to provide clean, short, simple but powerful interfaces to help
C++ engineers learn how C++ coroutines work.


## Acknowledgments

The design of this library is heavily influenced by:
- `cppcoro` library by Lewis Baker, https://github.com/lewissbaker/cppcoro
- "Asymmetric Transfer" blog by Lewis Baker, https://lewissbaker.github.io
- "Understanding C++ coroutines by example" by Pavel Novikov on C++ London, https://www.youtube.com/watch?v=7sKUAyWXNHA


## Differences from `cppcoro`

### General

- Compatible with the C++20 Standard rather then Coroutines TS specification
- CMake usage allows better adoptability
- Usage of C++20 synchronization features increase portability
- Easier, stronger, simpler interfaces and implementation thanks to
  - RAII usage for managing coroutine lifetime
  - usage of C++20 synchronization features
  - usage of C++20 concepts
  - usage of other C++20 features (i.e. spaceship operator)
  - using `[[nodiscard]]` attributes in much more places
  - `synchronized_task<Sync, T>` class template
    - task coroutine return type for handling non-standard (coroutine-like) synchronization
    - takes a synchronization primitive as the first template parameter
    - used in `sync_wait` and `when_all`
  - `storage<T>` class template
    - responsible for storage of the result or the current exception
    - implementation uses `std::variant` under the hood
    - interface similar to `std::promise`/`std::future` pair
    - could be replaced with `std::expected` proposed in [P0323](https://wg21.link/p0323) in the future
    - used in `task`, `synchronized_task`, and `async`
  - `nonvoid_storage<T>` class template
    - returns `void_type` for a task of `void`
    - needed for `when_all`
  - `task_promise_storage<T>` class template
    - separates coroutine promise logic from its storage
    - used in `task` and `synchronized_task`
    - NOTE: It is an undefined behavior to have `return_value()` and `return_result` in a coroutine
      promise type (even if mutually exclusive thanks to constraining with C++20 concepts) :-(


### `task<T>`

- Not default-constructible
- Internal storage is not mutable for task lvalues (`const` reference returned to the user)

```cpp
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
```

### `sync_wait()`

- Uses `std::binary_semaphore` for synchronization
- Much cleaner and shorter design


### `when_all()`

- Returns and instance of `void_type` in a tuple of results in case of `awaitable_of<void>`
- Much cleaner and shorter design


### `generator`

- Not default-constructible
- Produced values are not mutable (`const_iterator` returned to the user)
- As this is lazy synchronous generator a promise type does not waste space for storing
  `std::exception_ptr` but instead rethrows the exception right away
  - also no branches are taken in `begin()` and `operator++` to check if an exception should
    be re-thrown
- Returns `std::default_sentinel_t` from `end()` which immediately makes it usable with
  `std::counted_iterator` and possibly other facilities

```cpp
static_assert(!awaitable<generator<int>>);
static_assert(std::ranges::input_range<generator<int>>);
```

```cpp
mp_coro::generator<std::uint64_t> fibonacci()
{
  std::uint64_t a = 0, b = 1;
  while (true) {
    co_yield b;
    a = std::exchange(b, a + b);
  }
}

void f()
{
  auto gen = fibonacci();
  for(auto i : std::views::counted(gen.begin(), 10))
    std::cout << i << ' ';
  std::cout << '\n';
}
```


## New features

### C++20 Concepts

#### `awaiter<T>`

A concept that indicates a type contains the `await_ready()`, `await_suspend()` and `await_resume()`
member functions required to implement the protocol for suspending/resuming an awaiting coroutine.

#### `awaiter_of<T, Value>`

A concept that ensures that type `T` is an awaiter and that `await_resume()` returns `Value` type.

#### `awaitable<T>`

A concept that indicates that a type can be `co_await`ed in a coroutine context that has no
`await_transform()` overloads.

Any type that implements the `awaiter<T>` concept also implements the `awaitable<T>` concept.

#### `awaitable_of<T, Value>`

A concept that ensures that type `T` is an awaitable and that `await_resume()` returns `Value` type.

For example, the type `task<T>` implements the concept `awaitable_of<T&&>` whereas the type
`task<T>&` implements the concept `awaitable_of<const T&>`.

#### `task_result<T>`

A concept that ensures that the task result type is either `std::move_constructible` or a
reference or a `void` type.


### `coro_ptr`

A `std::unique_ptr` with a custom deleter.


### `async`

Awaitable that allows to asynchronously `co_await` on any invocable. More efficient than `std::async`
as it never allocates memory for shared `std::promise`/`std::future` storage.

NOTE: `async` should be `co_await`ed only once and that is why it works only for rvalues.

```cpp
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
```


### `TRACE_FUNC()`

A macro used across the library to facilitate debugging and learning of coroutines workflow.
