// std_asp_core.cpp
//
#include <detail/atomic_shared_ptr.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include <tuple>


using namespace detail::__std;

struct StdAtomicSharedPtrCore
: public ::testing::Test
{};


TEST_F(StdAtomicSharedPtrCore, constructible)
{
  { atomic_shared_ptr<int> asp; }
  { atomic_shared_ptr<int> asp(std::shared_ptr<int>()); }
  { atomic_shared_ptr<int> asp(std::make_shared<int>(13)); }
}


TEST_F(StdAtomicSharedPtrCore, is_lock_free)
{
  atomic_shared_ptr<int> asp;
  std::shared_ptr<int> sptr;
  ASSERT_EQ(std::atomic_is_lock_free(&sptr), asp.is_lock_free());
}


struct StdAtomicSharedPtrSimpleOps
: public ::testing::TestWithParam<std::tuple<
  std::shared_ptr<int>
, std::memory_order
>>
{};



TEST_P(StdAtomicSharedPtrSimpleOps, load_returns_current_sptr)
{
  auto const params = GetParam();
  auto const expected_sptr = std::get<0>(params);
  atomic_shared_ptr<int> asp(expected_sptr);

  auto const mo = std::get<1>(params);
  auto const actual_sptr = asp.load(mo);
  ASSERT_EQ(expected_sptr.get(), actual_sptr.get());
}


TEST_P(StdAtomicSharedPtrSimpleOps, sptr_conversion_returns_current_sptr)
{
  auto const params = GetParam();
  auto const expected_sptr = std::get<0>(params);
  atomic_shared_ptr<int> asp(expected_sptr);

  auto const actual_sptr = static_cast<std::shared_ptr<int>>(asp);
  ASSERT_EQ(expected_sptr.get(), actual_sptr.get());
}


TEST_P(StdAtomicSharedPtrSimpleOps, store_makes_sptr_current)
{
  auto const params = GetParam();
  auto const expected_sptr = std::get<0>(params);
  atomic_shared_ptr<int> asp;

  auto const mo = std::get<1>(params);
  asp.store(expected_sptr, mo);
  auto const actual_sptr = asp.load();
  ASSERT_EQ(expected_sptr.get(), actual_sptr.get());
}


TEST_P(StdAtomicSharedPtrSimpleOps, exchange_makes_sptr_current_and_returns_previous)
{
  auto const params = GetParam();
  auto const initial_sptr = std::get<0>(params);
  atomic_shared_ptr<int> asp(initial_sptr);

  auto const mo = std::get<1>(params);
  auto const new_sptr = std::make_shared<int>(13);
  auto const prev_sptr = asp.exchange(new_sptr, mo);
  ASSERT_EQ(prev_sptr.get(), initial_sptr.get());

  auto const actual_sptr = asp.load();
  ASSERT_EQ(new_sptr.get(), actual_sptr.get());
}

using SptrVec = std::vector<std::shared_ptr<int>>;
using MOVec = std::vector<std::memory_order>;

SptrVec const Sptrs = { std::shared_ptr<int>(), std::make_shared<int>(21) };
MOVec const MemoryOrders = 
{
  std::memory_order_seq_cst
, std::memory_order_acq_rel
, std::memory_order_acquire
, std::memory_order_release
, std::memory_order_consume
, std::memory_order_relaxed
};

INSTANTIATE_TEST_CASE_P( SimpleAtomicOperations
                       , StdAtomicSharedPtrSimpleOps
                       , ::testing::Combine( ::testing::ValuesIn(Sptrs)
                                           , ::testing::ValuesIn(MemoryOrders) ));

