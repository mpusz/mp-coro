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

cmake_minimum_required(VERSION 3.15)
project(mp-coro
    VERSION 0.1.0
    LANGUAGES CXX
)

set(projectPrefix MP_CORO_)

set(${projectPrefix}TRACE_LEVEL OFF CACHE STRING "Select downcasting mode")
set_property(CACHE ${projectPrefix}TRACE_LEVEL PROPERTY STRINGS OFF ON_ENTER ON_ENTER_AND_EXIT)

option(${projectPrefix}AS_SYSTEM_HEADERS "Exports library as system headers" OFF)
message(STATUS "${projectPrefix}AS_SYSTEM_HEADERS: ${${projectPrefix}AS_SYSTEM_HEADERS}")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

if(${projectPrefix}AS_SYSTEM_HEADERS)
    set(coroAsSystem SYSTEM)
endif()

add_library(mp-coro
    include/mp-coro/async.h
    include/mp-coro/concepts.h
    include/mp-coro/coro_ptr.h
    include/mp-coro/generator.h
    include/mp-coro/sync_await.h
    include/mp-coro/task.h
    include/mp-coro/trace.h
    include/mp-coro/type_traits.h
    include/mp-coro/cancellation.h

    lib/mp-coro/bits/cancellation_registry.cpp
    lib/mp-coro/bits/cancellation_state.cpp
)
target_compile_features(mp-coro PUBLIC cxx_std_20)
target_include_directories(mp-coro ${coroAsSystem}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
add_library(mp-coro::mp-coro ALIAS mp-coro)

# installation
install(TARGETS mp-coro EXPORT mp-coroTargets)
install(DIRECTORY include/units TYPE INCLUDE)

install(EXPORT mp-coroTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mp-coro
    NAMESPACE mp-coro::
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(mp-coroConfigVersion.cmake COMPATIBILITY SameMajorVersion)

install(FILES mp-coroConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/mp-coroConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mp-coro
)

if(DEFINED ${projectPrefix}TRACE_LEVEL)
    set(trace_level_options OFF ON_ENTER ON_ENTER_AND_EXIT)
    list(FIND trace_level_options "${${projectPrefix}TRACE_LEVEL}" trace_level)
    if(trace_level EQUAL -1)
        message(FATAL_ERROR "'${projectPrefix}TRACE_LEVEL' should be one of ${trace_level_options} ('${${projectPrefix}TRACE_LEVEL}' received)")
    else()
        message(STATUS "${projectPrefix}TRACE_LEVEL: ${${projectPrefix}TRACE_LEVEL}")
        target_compile_definitions(mp-coro INTERFACE ${projectPrefix}TRACE_LEVEL=${trace_level})
    endif()
endif()
