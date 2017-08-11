#pragma once

#include <memory>
#include <atomic>

namespace detail { namespace __std {

template< typename T >
class atomic_shared_ptr {
public:
  constexpr atomic_shared_ptr() noexcept = default;
  constexpr atomic_shared_ptr(std::shared_ptr<T> desired) noexcept
  : sptr{desired}
  {}

  atomic_shared_ptr(const atomic_shared_ptr&) = delete;

  void operator= (std::shared_ptr<T> desired) noexcept
  { store(desired); }

  void operator= (const atomic_shared_ptr&) = delete;

  bool is_lock_free() const noexcept
  { return std::atomic_is_lock_free(&sptr); }

  void store(std::shared_ptr<T> desired, std::memory_order order = std::memory_order_seq_cst) noexcept
  { std::atomic_store_explicit(&sptr, desired, order); }

  std::shared_ptr<T> load(std::memory_order order = std::memory_order_seq_cst) const noexcept
  { return std::atomic_load_explicit(&sptr, order); }

  operator std::shared_ptr<T>() const noexcept
  { return load(); }

  std::shared_ptr<T> exchange(std::shared_ptr<T> desired, std::memory_order order = std::memory_order_seq_cst ) noexcept
  { return std::atomic_exchange_explicit(&sptr, desired, order); }

  bool compare_exchange_weak(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired,
                             std::memory_order success,    std::memory_order failure) noexcept
  { return std::atomic_compare_exchange_weak_explicit(&sptr, &expected, desired, success, failure); }

  bool compare_exchange_weak(std::shared_ptr<T>& expected, std::shared_ptr<T>&& desired,
                             std::memory_order success,    std::memory_order failure) noexcept
  { return std::atomic_compare_exchange_weak_explicit(&sptr, &expected, desired, success, failure); }

  bool compare_exchange_weak(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired,
                             std::memory_order order = std::memory_order_seq_cst) noexcept
  { return compare_exchange_weak(expected, desired, order, order); }

  bool compare_exchange_weak(std::shared_ptr<T>& expected, std::shared_ptr<T>&& desired,
                             std::memory_order order = std::memory_order_seq_cst) noexcept
  { return compare_exchange_weak(expected, std::move(desired), order, order); }


  bool compare_exchange_strong(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired,
                               std::memory_order success,    std::memory_order failure) noexcept
  { return std::atomic_compare_exchange_strong_explicit(&sptr, &expected, desired, success, failure); }

  bool compare_exchange_strong(std::shared_ptr<T>& expected, std::shared_ptr<T>&& desired,
                               std::memory_order success,    std::memory_order failure) noexcept
  { return std::atomic_compare_exchange_strong_explicit(&sptr, &expected, desired, success, failure); }

  bool compare_exchange_strong(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired,
                               std::memory_order order = std::memory_order_seq_cst) noexcept
  { return compare_exchange_strong(expected, desired, order, order); }

  bool compare_exchange_strong(std::shared_ptr<T>& expected, std::shared_ptr<T>&& desired,
                               std::memory_order order = std::memory_order_seq_cst) noexcept
  { return compare_exchange_strong(expected, std::move(desired), order, order); }

private:
  std::shared_ptr<T> sptr;

};


} // namespace __std
} // namespace detail

