#include <mp-coro/bits/cancellation_registry.h>
#include <mp-coro/bits/constant_backoff.h>
#include <mp-coro/cancellation.h>
#include <cstdint>

namespace {

static constexpr auto n_bins = mp_coro::detail::cancellation_registry<>::n_bins;
static constexpr auto inhabited_flags_begin = mp_coro::detail::cancellation_registry<>::inhabited_flags_begin;
static constexpr auto availability_mask = mp_coro::detail::cancellation_registry<>::availability_mask;

// ==== operations to query availability_flags_ ====

std::uint64_t availability(std::uint64_t availability_flags) { return availability_mask & availability_flags; }

std::uint64_t inhabiting(std::uint64_t availability_flags) { return availability_flags >> n_bins; }

std::uint64_t inhabited_and_available(std::uint64_t availability_flags)
{
  return availability(availability_flags) & inhabiting(availability_flags);
}

// ==== operations to find availability or inhabit state of a specific bit ====

std::int64_t first_inhabited_and_available(std::uint64_t availability_flags)
{
  return std::countr_zero(inhabited_and_available(availability_flags));
}

std::int64_t first_available(std::uint64_t availability_flags)
{
  return std::countr_zero(availability(availability_flags));
}

std::int64_t first_inhabited(std::uint64_t availability_flags)
{
  return std::countr_zero(inhabiting(availability_flags));
}

std::uint64_t availability_mask_from_idx(std::int64_t idx) { return UINT64_C(1) << idx; }

std::uint64_t inhabit_mask_from_idx(std::int64_t idx) { return inhabited_flags_begin << idx; }

bool is_bin_available(std::uint64_t availability_flags, std::int64_t idx)
{
  return (availability(availability_flags) & availability_mask_from_idx(idx)) > 0;
}

std::int64_t query_availability_from_idx(std::uint64_t availability_flags, std::int64_t idx)
{  // Probe all bins for availability (starting from idx). If no available bin is found, return whatever index is left
   // at idx after exiting the probing loop.
  for (std::int64_t attempts = 0; attempts < n_bins && !is_bin_available(availability_flags, idx); ++attempts)
    idx = (idx + attempts) % n_bins;
  return idx;
}

std::int64_t thread_randomized_bin_idx()
{
  // Randomize index of bin to be used (reduce contention)
  return std::hash<std::thread::id>{}(std::this_thread::get_id()) % n_bins;
}
}  // namespace

// ==== cancellation_register function members ====

namespace mp_coro::detail {

template<std::int64_t max_bin_size>
bool cancellation_registry<max_bin_size>::empty() const
{
  return inhabiting(availability_flags_.load(std::memory_order_acquire)) == 0;
}

template<std::int64_t max_bin_size>
std::optional<cancellation_registration*> cancellation_registry<max_bin_size>::try_deregister_one()
{  // Variables used in the CAS loop:
  // - Index of the bin to be used - initially an invalid index.
  // - Use flag_op to operate with availability_flags.
  // - Do standard procedure of CAS loop with old and new copy.
  std::int64_t idx = n_bins;
  std::uint64_t old_availability_flags = availability_flags_.load(std::memory_order_acquire);
  std::uint64_t new_availability_flags;
  std::uint64_t flag_op;
  do {  // CAS loop
    // Must check idx < n_bins inside CAS loop to avoid UB (trying to shift in bit_mask_from_idx when idx is greater
    // than n_bins). Do not change old availability_flags if extraction cannot be done.
    idx = first_inhabited_and_available(old_availability_flags);
    flag_op = 1;
    new_availability_flags = old_availability_flags;
    if (idx < n_bins) new_availability_flags ^= (flag_op = availability_mask_from_idx(idx));
  } while (!availability_flags_.compare_exchange_weak(old_availability_flags, new_availability_flags,
                                                      std::memory_order_acq_rel));
  // Now this thread has exclusive access to bins_[idx].
  if (idx >= n_bins) return std::nullopt;

  auto ret = [&]() -> std::optional<cancellation_registration*> {
    // std::make_optional may throw.
    // If bin remains empty after extraction, set inhabited bit for toggling.
    try {
      auto x = std::make_optional(bin_extract(bins_[idx]));
      if (bin_empty(bins_[idx])) flag_op |= inhabit_mask_from_idx(idx);
      return x;
    } catch (const std::exception& e) {
      return std::nullopt;  // NOTE: what should be done for std::bad_alloc? Probably propagate the exception.
    }
  }();

  // Toggle availability and inhabited bit if bin ends up empty. Publish changes to other threads.
  availability_flags_.fetch_xor(flag_op, std::memory_order_release);
  return ret;
}

template<std::int64_t max_bin_size>
void cancellation_registry<max_bin_size>::deregister(cancellation_registration* x)
{
  std::int64_t idx = x->bin_idx;
  std::uint64_t idx_mask = availability_mask_from_idx(idx);
  constant_backoff backoff{};
  auto do_cas = [&]() -> bool {  // Return true if CAS and exclusivity of bins_[idx] is achieved, false otherwise.
    // NOTE: this CAS loop is a bit different than the usual do...while idiom because this avoids issuing an extra call
    // to backoff() on the first iteration.
    std::uint64_t old_availability_flags = availability_flags_.load(std::memory_order_acquire);
    std::uint64_t flag_op = is_bin_available(old_availability_flags, idx) ? idx_mask : 0;
    std::uint64_t new_availability_flags = old_availability_flags ^ flag_op;
    return availability_flags_.compare_exchange_weak(old_availability_flags, new_availability_flags,
                                                     std::memory_order_acq_rel) &&
           new_availability_flags != old_availability_flags;
  };
  while (!do_cas()) backoff();
  // Now this thread has exclusive access to bins_[idx].

  if (auto& bin = bins_[idx]; !bin_empty(bin)) {
    bin_remove(x, bin);
    if (bin_empty(bin)) idx_mask |= inhabit_mask_from_idx(idx);
  }

  // Toggle availability and inhabited bit if bin ends up empty. Publish changes to other threads.
  availability_flags_.fetch_xor(idx_mask, std::memory_order_release);
};

template<std::int64_t max_bin_size>
void cancellation_registry<max_bin_size>::force_register(cancellation_registration* x)
{  // Change the starting bin index between attempts.

  // NOTE 1: Maybe there is a better operation than just increasing idx by one on faliure.
  // NOTE 2: Some backoff can be implemented in here. I've tried constant backoff and it performs poorly when the
  // number of threads is low (and it doesn't seem to be worth it for larger number of threads - I've tested up to
  // 1024 threads -).
  std::int64_t idx = thread_randomized_bin_idx();
  while (!try_register_from_idx(x, idx)) idx = (idx + 1) % n_bins;
}

template<std::int64_t max_bin_size>
bool cancellation_registry<max_bin_size>::try_register(cancellation_registration* x)
{
  return try_register_from_idx(x, thread_randomized_bin_idx());
}

template<std::int64_t max_bin_size>
bool cancellation_registry<max_bin_size>::try_register_from_idx(cancellation_registration* x, std::int64_t idx)
{  // Variables used in the CAS loop:
  // - Use flag_op to operate with availability_flags.
  // - Do standard procedure of CAS loop with old and new copy.
  std::uint64_t old_availability_flags = availability_flags_.load(std::memory_order_acquire);
  std::uint64_t new_availability_flags;
  std::uint64_t flag_op;
  do {  // CAS loop
    // Try to find an available bin. If there is no available bin, then leave availability_flags unchanged (0 is XOR
    // identity) and force boolean do_insertion evaluation to false by setting idx = n_bins.
    idx = query_availability_from_idx(old_availability_flags, idx);
    if (is_bin_available(old_availability_flags, idx)) {
      flag_op = availability_mask_from_idx(idx);
    } else {
      flag_op = 0;
      idx = n_bins;
    }
    new_availability_flags = old_availability_flags ^ flag_op;
  } while (!availability_flags_.compare_exchange_weak(old_availability_flags, new_availability_flags,
                                                      std::memory_order_acq_rel));
  // Now this thread has exclusive access to bins_[idx].
  bool do_insertion = idx < n_bins && !bin_maxed_out(bins_[idx]);

  if (do_insertion) {
    try {  // Insertion may throw. If it succeeds, mark bin as inhabited.
      bin_insert_at_idx(x, idx);
      x->bin_idx = idx;
      flag_op |= inhabit_mask_from_idx(idx);
    } catch (const std::exception& e) {  // Else, insertion failed.
      do_insertion = false;  // NOTE: what should be done for std::bad_alloc? Probably propagate the exception.
    }
  }

  // Publish changes to other threads.
  availability_flags_.fetch_or(flag_op, std::memory_order_release);
  return do_insertion;
}

}  // namespace mp_coro::detail
