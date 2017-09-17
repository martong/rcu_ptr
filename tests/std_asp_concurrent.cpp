// std_asp_concurrent.cpp
//
#include <detail/atomic_shared_ptr.hpp>
#include <tests/scoped_thread.hpp>
#include <tests/countdown.hpp>
#include <tests/orders.hpp>

#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <tuple>

namespace test {

using namespace detail::__std;
using MO = std::memory_order;
using LoadOrder = MO;
using StoreOrder = MO;
using Thread = tools::scoped_thread<std::thread>;

struct StdAtomicSharedPtrLoadStoresTest
    : public ::testing::TestWithParam<std::tuple<LoadOrder, StoreOrder>> {};

TEST_P(StdAtomicSharedPtrLoadStoresTest, concurrent_stores_w_loads) {
    auto const params = GetParam();
    auto const load_o = std::get<0>(params);
    auto const store_o = std::get<1>(params);

    atomic_shared_ptr<int> asp(std::make_shared<int>(21));
    std::atomic<bool> go{false};
    auto const N = 2;
    auto iterations = tools::Countdown{100};

    auto reader = [&go, &asp, iterations, load_o ]() mutable noexcept {
        while (!go) { /*spin*/
        }
        while (iterations--) {
            auto const sptr = asp.load(load_o);
            EXPECT_TRUE(static_cast<bool>(sptr));
        }
    };

    auto const expected_value = 13;
    auto writer =
        [&go, &asp, iterations, expected_value, store_o ]() mutable noexcept {
        while (!go) { /*spin*/
        }
        while (iterations--) {
            auto const sptr = std::make_shared<int>(expected_value);
            asp.store(sptr, store_o);
        }
    };
    {
        std::vector<Thread> threads;
        threads.reserve(N * 2);
        for (auto i = 0; i < N; i++) {
            threads.emplace_back(std::thread(reader));
            threads.emplace_back(std::thread(writer));
        }
        go = true;
    }

    auto const sptr = asp.load(load_o);
    ASSERT_TRUE(static_cast<bool>(sptr));
    ASSERT_EQ(expected_value, *sptr);
}

INSTANTIATE_TEST_CASE_P(
    Concurrent, StdAtomicSharedPtrLoadStoresTest,
    ::testing::Combine(::testing::ValuesIn(mo::LoadOrders),
                       ::testing::ValuesIn(mo::StoreOrders)), );

using RMWOrder = std::memory_order;

struct StdAtomicSharedPtrReadModifyWriteTest
    : public ::testing::TestWithParam<
          std::tuple<LoadOrder, StoreOrder, RMWOrder>> {
    using ParamsType = std::tuple<LoadOrder, StoreOrder, RMWOrder>;

    template <typename LF, typename SF, typename RMWF>
    static void Test(ParamsType const &Params, LF &&LoadFn, SF &&StoreFn,
                     RMWF &&RMWFn) {
        static auto const N = 2;
        auto const initial = std::make_shared<int>(13);
        atomic_shared_ptr<int> asp(initial);
        std::atomic<bool> go{false};

        auto iterations = tools::Countdown{10};
        auto const load_o = std::get<0>(Params);
        auto const store_o = std::get<1>(Params);
        auto const rmw_o = std::get<2>(Params);

        auto reader = [&, iterations ]() mutable noexcept {
            while (!go)
                ;
            while (iterations--) LoadFn(asp, load_o);
        };

        auto writer = [&, iterations ]() mutable noexcept {
            while (!go)
                ;
            while (iterations--) StoreFn(asp, store_o);
        };

        auto rmw = [&, iterations ]() mutable noexcept {
            while (!go)
                while (iterations--) RMWFn(asp, rmw_o);
        };
        {
            std::vector<Thread> threads;
            threads.reserve(N * 3);
            for (auto i = 0; i < N; i++) {
                threads.emplace_back(reader);
                threads.emplace_back(writer);
                threads.emplace_back(rmw);
            }
            go = true;
        }
    }
};

TEST_P(StdAtomicSharedPtrReadModifyWriteTest, load_store_exchange) {
    auto const params = GetParam();
    Test(params,
         [](auto &Asp, auto const LO) mutable noexcept {
             auto const sptr = Asp.load(LO);
             ASSERT_TRUE(static_cast<bool>(sptr));
         }

         ,
         [](auto &Asp, auto const SO) mutable noexcept {
             auto const sptr = std::make_shared<int>(21);
             Asp.store(sptr, SO);
         }

         ,
         [](auto &Asp, auto const RMWO) mutable noexcept {
             auto const new_ = std::make_shared<int>(31);
             auto const prev = Asp.exchange(new_, RMWO);
             ASSERT_TRUE(static_cast<bool>(prev));
         });
}

TEST_P(StdAtomicSharedPtrReadModifyWriteTest, load_store_cas_strong) {
    auto const params = GetParam();
    Test(params,
         [](auto &Asp, auto const LO) mutable noexcept {
             auto const sptr = Asp.load(LO);
             ASSERT_TRUE(static_cast<bool>(sptr));
         }

         ,
         [](auto &Asp, auto const SO) mutable noexcept {
             auto const sptr = std::make_shared<int>(21);
             Asp.store(sptr, SO);
         }

         ,
         [](auto &Asp, auto const RMWO) mutable noexcept {
             auto const desired = std::make_shared<int>(42);
             auto expected = Asp.load(RMWO);
             do {
                 ASSERT_TRUE(static_cast<bool>(expected));
             } while (
                 !Asp.compare_exchange_strong(expected, desired, RMWO, RMWO));
         });
}

TEST_P(StdAtomicSharedPtrReadModifyWriteTest, load_store_cas_weak) {
    auto const params = GetParam();
    Test(
        params,
        [](auto &Asp, auto const LO) mutable noexcept {
            auto const sptr = Asp.load(LO);
            ASSERT_TRUE(static_cast<bool>(sptr));
        }

        ,
        [](auto &Asp, auto const SO) mutable noexcept {
            auto const sptr = std::make_shared<int>(21);
            Asp.store(sptr, SO);
        }

        ,
        [](auto &Asp, auto const RMWO) mutable noexcept {
            auto const desired = std::make_shared<int>(42);
            auto expected = Asp.load(RMWO);
            do {
                ASSERT_TRUE(static_cast<bool>(expected));
            } while (!Asp.compare_exchange_weak(expected, desired, RMWO, RMWO));
        });
}

INSTANTIATE_TEST_CASE_P(
    ConcurrentRMWOperations, StdAtomicSharedPtrReadModifyWriteTest,
    ::testing::Combine(::testing::ValuesIn(mo::LoadOrders),
                       ::testing::ValuesIn(mo::StoreOrders),
                       ::testing::ValuesIn(mo::ReadModifyWriteOrders)), );

} // namespace test
