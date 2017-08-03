// rcu_unit.cpp
//
#include <rcu_ptr.hpp>
#include <gtest/gtest.h>


struct RCUPtrCoreTest
: public ::testing::Test
{};


TEST_F(RCUPtrCoreTest, default_constructible)
{
  rcu_ptr<int> p;
  (void)p;
}


TEST_F(RCUPtrCoreTest, resetable)
{
    auto p = rcu_ptr<int>{};
    auto const new_ = std::make_shared<int>(42);
    p.reset(new_);

    auto const current = p.read();
    ASSERT_TRUE(static_cast<bool>(current));
    ASSERT_EQ(42, *current);
}


TEST_F(RCUPtrCoreTest, empty_rcu_ptr_calls_with_nullptr)
{
    auto p = rcu_ptr<int>{};
    p.copy_update([](auto cp) { ASSERT_EQ(cp, nullptr); });
}


TEST_F(RCUPtrCoreTest, empty_rcu_ptr_reset_inside_copy_update)
{
    auto p = rcu_ptr<int>{};
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

