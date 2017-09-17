// rcu_unit.cpp
//
#include <rcu_ptr.hpp>
#include <gtest/gtest.h>

struct RCUPtrCoreTest : public ::testing::Test {};

TEST_F(RCUPtrCoreTest, default_constructible) {
    rcu_ptr<int> p;
    (void)p;
}

TEST_F(RCUPtrCoreTest, resetable) {
    rcu_ptr<int> p;
    auto const new_ = std::make_shared<int>(42);
    p.reset(new_);

    auto const current = p.read();
    ASSERT_TRUE(static_cast<bool>(current));
    ASSERT_EQ(42, *current);
}

TEST_F(RCUPtrCoreTest, empty_rcu_ptr_calls_with_nullptr) {
    rcu_ptr<int> p;
    p.copy_update([](auto cp) { ASSERT_EQ(cp, nullptr); });
}

TEST_F(RCUPtrCoreTest, empty_rcu_ptr_reset_inside_copy_update) {
    rcu_ptr<int> p;
    p.copy_update([&p](auto cp) {
        if (cp == nullptr) {
            p.reset(std::make_shared<int>(42));
        } else {
            ++(*cp);
        }
    });

    auto const current = p.read();
    ASSERT_TRUE(static_cast<bool>(current));
    ASSERT_EQ(43, *current);
}
