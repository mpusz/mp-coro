#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <functional>
#include <optional>
#include <thread>
#include <vector>

namespace mp_coro {

struct cancellation_registration;

namespace detail {
template<std::int64_t max_bin_size = 1024>
struct cancellation_registry {
  using value_type = std::add_pointer_t<cancellation_registration>;
  using bin_t = std::vector<value_type>;

  // n_bins == 32 allows having a 64bit state indicating with the first 32  bits if the bins are not taken by another
  // thread, and with the last 32 bits if a bin is inhabited (contains ones or more elements).
  static constexpr std::int64_t n_bins = 32;
  static constexpr std::uint64_t inhabited_flags_begin = UINT64_C(1) << n_bins;
  static constexpr std::uint64_t availability_mask = inhabited_flags_begin - 1;

  // Bin indices are handled with signed integers, masks and flags are handled with unsigned integers (for logical
  // shifts).
  std::array<bin_t, n_bins> bins_;
  std::atomic<std::uint64_t> availability_flags_ = availability_mask;

  bool empty() const;

  // Deregister a single `cancellation_registration` if possible.
  std::optional<cancellation_registration*> try_deregister_one();

  // Try deregistering a specific `cancellation_registration`.
  void deregister(cancellation_registration* x);

  // Try registering until it success.
  void force_register(cancellation_registration* x);

  // Try registering. Returns true if it succeeds, false otherwise.
  bool try_register(cancellation_registration* x);

private:
  // Try registering starting at bin `idx`. May pick another bin if bin `idx` is not available.
  // Return true if the insertion was succesful, otherwise false.
  bool try_register_from_idx(cancellation_registration* x, std::int64_t idx);

  // ==== operations on bins (should the underlying container type and its bin operations be CPOs of some sort) ====

  void bin_insert_at_idx(cancellation_registration* x, std::int64_t idx) { bin_insert(x, bins_[idx]); }
  static void bin_insert(cancellation_registration* x, bin_t& xs) { xs.push_back(x); }
  static void bin_remove(cancellation_registration* x, bin_t& xs)
  {
    xs.erase(std::remove(xs.begin(), xs.end(), x), xs.end());
  }
  [[nodiscard]] static bool bin_maxed_out(const bin_t& xs) { return xs.size() > max_bin_size; }
  [[nodiscard]] static bool bin_empty(const bin_t& xs) { return xs.size() == 0; }
  [[nodiscard]] static cancellation_registration* bin_extract(bin_t& xs)
  {
    auto x = xs.back();
    xs.pop_back();
    return x;
  }
};
}  // namespace detail
}  // namespace mp_coro
