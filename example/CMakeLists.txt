# The MIT License (MIT)
#
# Copyright (c) 2021 Mateusz Pusz
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.10)

#
# add_example(target <depependencies>...)
#
function(add_example target)
    add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} PRIVATE ${ARGN})
endfunction()

find_package(Threads REQUIRED)

add_example(async_read_file mp-coro::mp-coro Threads::Threads)
add_example(concepts mp-coro::mp-coro)
add_example(generator mp-coro::mp-coro)
add_example(run_async mp-coro::mp-coro Threads::Threads)
add_example(simple_async_tasks mp-coro::mp-coro Threads::Threads)
add_example(simple_tasks mp-coro::mp-coro)
add_example(sleep_for mp-coro::mp-coro)
add_example(when_all mp-coro::mp-coro Threads::Threads)
