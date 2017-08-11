// std_asp_concurrent.cpp
//
#include <detail/atomic_shared_ptr.hpp>
#include <tests/scoped_thread.hpp>
#include <tests/orders.hpp>

#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <tuple>


namespace test {

using namespace detail::__std;
using MO = std::memory_order;
using LoadOrder  = MO;
using StoreOrder = MO;
using Thread = tools::scoped_thread<std::thread>;


struct StdAtomicSharedPtrLoadStores
: public ::testing::TestWithParam<std::tuple<
  LoadOrder
, StoreOrder
>>
{};


TEST_P(StdAtomicSharedPtrLoadStores, concurrent_stores_w_loads)
{
  auto const params  = GetParam();
  auto const load_o  = std::get<0>(params);
  auto const store_o = std::get<1>(params);

  atomic_shared_ptr<int> asp(std::make_shared<int>(21));
  std::atomic<bool> go{false};
  auto const N = 2;
  auto Iterations = 100;

  auto reader = [&go, &asp, Iterations, load_o] () mutable noexcept
  {
    while (! go) {/*spin*/}
    while (Iterations--)
    { auto const sptr = asp.load(load_o); EXPECT_TRUE(static_cast<bool>(sptr)); }
  };

  auto const expected_value = 13;
  auto writer = [&go, &asp, Iterations, expected_value, store_o] () mutable noexcept
  {
    while (! go) {/*spin*/}
    while (Iterations--)
    {
      auto const sptr = std::make_shared<int>(expected_value);
      asp.store(sptr, store_o);
    }
  };
  {
    std::vector<Thread> threads;
    threads.reserve( N*2 );
    for( auto i = 0; i < N; i++ )
    {
      threads.emplace_back(std::thread(reader));
      threads.emplace_back(std::thread(writer));
    }
    go = true;
  }

  auto const sptr = asp.load(load_o);
  ASSERT_TRUE(static_cast<bool>(sptr));
  ASSERT_EQ(expected_value, *sptr);
}


INSTANTIATE_TEST_CASE_P( Concurrent
                       , StdAtomicSharedPtrLoadStores
                       , ::testing::Combine( ::testing::ValuesIn(mo::LoadOrders), ::testing::ValuesIn(mo::StoreOrders) ));


} // namespace test

