// orders.hpp
//
#pragma once
#include <memory>
#include <array>

namespace mo {

std::array<std::memory_order, 4> constexpr LoadOrders =
{
  std::memory_order_seq_cst
, std::memory_order_acquire
, std::memory_order_consume
, std::memory_order_relaxed
};

std::array<std::memory_order, 3> constexpr StoreOrders = 
{
  std::memory_order_seq_cst
, std::memory_order_release
, std::memory_order_relaxed
};

std::array<std::memory_order, 5> constexpr ReadModifyWriteOrders = 
{
  std::memory_order_seq_cst
, std::memory_order_acq_rel
, std::memory_order_acquire
, std::memory_order_release
, std::memory_order_relaxed
};

} // namespace mo

