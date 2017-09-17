// rcu_race.cpp
//
#include <rcu_ptr.hpp>
#include <tests/ExecuteInLoop.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <thread>

struct RCUPtrRaceTest : public ::testing::Test {};

#if 0
TEST_F(RCUPtrRaceTest, tsan_catches_data_race)
{
    int a;

    std::thread t1{[&a]() { executeInLoop<1000>([&a]() { ++a; }); }};
    executeInLoop<1000>([&a]() { --a; });

    t1.join();
}
#endif

TEST_F(RCUPtrRaceTest, read_reset) {
    rcu_ptr<int> p(std::make_shared<int>(42));

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            auto const new_ = std::make_shared<int>(42);
            p.reset(new_);
        });
    }};

    executeInLoop<10000>([&p]() {
        auto& x = *p.read();
        (void)x;
    });

    t1.join();
}

TEST_F(RCUPtrRaceTest, read_copy_update) {
    rcu_ptr<int> p(std::make_shared<int>(42));

    std::thread t1{[&p]() {
        executeInLoop<10000>(
            [&p]() { p.copy_update([](auto cp) { *cp = 42; }); });
    }};

    executeInLoop<10000>([&p]() {
        auto& x = *p.read();
        (void)x;
    });

    t1.join();

    auto const current = p.read();
    ASSERT_TRUE(static_cast<bool>(current));
    ASSERT_EQ(42, *current);
}

TEST_F(RCUPtrRaceTest, reset_reset) {
    rcu_ptr<int> p;

    auto l = [&p]() {
        executeInLoop<10000>([&p]() {
            auto const new_ = std::make_shared<int>(42);
            p.reset(new_);
        });
    };

    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();
}

// void test_read_update() {
// auto p = rcu_ptr<int>{};

// std::thread t1{[&p]() {
// executeInLoop<10000>([&p]() {
// p.update([](const std::shared_ptr<const int>& v) {
// auto v2 = std::make_shared<int>(42);
// return v2;
//});
//});
//}};

// executeInLoop<10000>([&p]() { auto& x = *p.read(); });

// t1.join();
// ASSERT(*p.read() == 42);
//}

// void test_update_update() {
// auto p = rcu_ptr<int>{};
// p.update([](const std::shared_ptr<const int>& v) {
// return std::make_shared<int>(0);
//});

// std::thread t1{[&p]() {
// executeInLoop<10000>([&p]() {
// p.update([](const std::shared_ptr<const int>& v) {
// return std::make_shared<int>((*v) + 1);
//});
//});
//}};

// std::thread t2{[&p]() {
// executeInLoop<10000>([&p]() {
// p.update([](const std::shared_ptr<const int>& v) {
// return std::make_shared<int>((*v) + 1);
//});
//});
//}};

// t1.join();
// t2.join();
// ASSERT(*p.read() == 20000);
//}

TEST_F(RCUPtrRaceTest, copy_update_copy_update) {
    rcu_ptr<int> p(std::make_shared<int>(0));

    auto l = [&p]() {
        executeInLoop<10000>(
            [&p]() { p.copy_update([](auto copy) { (*copy)++; }); });
    };

    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();

    auto const current = p.read();
    ASSERT_TRUE(static_cast<bool>(current));
    ASSERT_EQ(20000, *current);
}

TEST_F(RCUPtrRaceTest, copy_update_push_back) {
    using V = std::vector<int>;
    rcu_ptr<V> p(std::make_shared<V>());
    const int i = 2;

    auto l = [&p, &i]() {
        executeInLoop<1000>([&p, &i]() {
            p.copy_update([&i](auto copy) { copy->push_back(i); });
        });
    };

    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();

    auto const current = p.read();
    ASSERT_TRUE(static_cast<bool>(current));
    ASSERT_EQ(2000ul, current->size());
}

TEST_F(RCUPtrRaceTest, read_read) {
    rcu_ptr<int> p(std::make_shared<int>(42));

    auto l = [&p]() {
        executeInLoop<10000>([&p]() {
            auto& x = *p.read();
            (void)x;
        });
    };

    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();
}

TEST_F(RCUPtrRaceTest, assign_reset) {
    rcu_ptr<int> p;

    std::thread t2{[&p]() {
        executeInLoop<10000>([&p]() { p = std::shared_ptr<int>(); });
    }};

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            auto const new_ = std::make_shared<int>(42);
            p.reset(new_);
        });
    }};

    t1.join();
    t2.join();
}

#if 0
TEST_F(RCUPtrRaceTest, copyctor_reset)
{
    rcu_ptr<int> p;

    std::thread t2{[&p]() { executeInLoop<10000>([&p]()
    { auto const p2 = p; (void)p2; }); }};

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            auto const new_ = std::make_shared<int>(42);
            p.reset(new_);
        });
    }};

    t1.join();
    t2.join();
}


TEST_F(RCUPtrRaceTest, copyctor_assign)
{
    rcu_ptr<int> p;

    std::thread t1{[&p]() { executeInLoop<10000>([&p]()
    { auto const p2 = p; (void)p2; }); }};

    std::thread t2{
        [&p]() { executeInLoop<10000>([&p]() { p = rcu_ptr<int>{}; }); }};

    t1.join();
    t2.join();
}
#endif

TEST_F(RCUPtrRaceTest, empty_rcu_ptr_reset_inside_copy_update) {
    rcu_ptr<int> p;
    auto l = [&p]() {
        p.copy_update([&p](auto cp) {
            if (cp == nullptr) {
                p.reset(std::make_shared<int>(42));
            } else {
                (*cp)++;
            }
        });
    };
    std::thread t1{[l]() { executeInLoop<1000>(l); }};
    std::thread t2{[l]() { executeInLoop<1000>(l); }};

    t1.join();
    t2.join();

    auto const current = p.read();
    ASSERT_TRUE(static_cast<bool>(current));
    std::cout << *current << std::endl;
    ASSERT_EQ(2042, *current);
}

class X {
    rcu_ptr<std::vector<int>> v;

public:
    X() { v.reset(std::make_shared<std::vector<int>>()); }
    int sum() const { // read operation
        std::shared_ptr<const std::vector<int>> local_copy = v.read();
        return std::accumulate(local_copy->begin(), local_copy->end(), 0);
    }
    void add(int i) { // write operation
        v.copy_update([i](auto copy) { copy->push_back(i); });
    }
};

TEST_F(RCUPtrRaceTest, sum_add) {
    X x;

    int sum = 0;
    std::thread t2{
        [&x, &sum]() { executeInLoop<1000>([&x, &sum]() { sum = x.sum(); }); }};

    std::thread t1{[&x]() { executeInLoop<1000>([&x]() { x.add(3); }); }};

    std::thread t3{[&x]() { executeInLoop<1000>([&x]() { x.add(4); }); }};

    t1.join();
    t2.join();
    t3.join();

    std::cout << x.sum() << std::endl;
    ASSERT_EQ(7000, x.sum());
}
