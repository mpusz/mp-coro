#pragma once

#include <chrono>
#include <cstdint>
#include <thread>

// Constant backoff helper class. Helpful when there is contention.
class constant_backoff {
  static constexpr std::uint32_t max_retries = 4000;
  static constexpr std::chrono::nanoseconds sleep_time = std::chrono::microseconds(300);
  uint32_t spin_counter = 0;

public:
  void operator()() noexcept
  {
    if (spin_counter < max_retries) {
      std::this_thread::yield();
      ++spin_counter;
    } else {
      std::this_thread::sleep_for(sleep_time);
    }
  }
};
