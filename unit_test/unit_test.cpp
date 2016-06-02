#include "../rcu_ptr.hpp"
#include <cassert>
#include <iostream>

#define ASSERT(CONDITION)                                                      \
    do                                                                         \
        if (!(CONDITION)) {                                                    \
            printf("Assertion failure %s:%d ASSERT(%s)\n", __FILE__, __LINE__, \
                   #CONDITION);                                                \
            abort();                                                           \
        }                                                                      \
    while (0)

void test() {
    auto p = rcu_ptr<int>{};
    auto new_ = std::make_shared<int>(42);
    p.reset(new_);
    ASSERT(42 == *p.read());
}

void test_uninitialized_rcu_ptr_calls_with_nullptr() {
    auto p = rcu_ptr<int>{};
    p.copy_update([](auto cp) { ASSERT(cp == nullptr); });
}

void test_uninitialized_rcu_ptr_reset_inside_copy_update() {
    auto p = rcu_ptr<int>{};
    p.copy_update([&p](auto cp) {
        if (cp == nullptr) {
            p.reset(std::make_shared<int>(42));
        } else {
            ++(*cp);
        }
    });
    ASSERT(43 == *p.read());
}

int main() {
    test();
    test_uninitialized_rcu_ptr_calls_with_nullptr();
    test_uninitialized_rcu_ptr_reset_inside_copy_update();
    std::cout << "OK" << std::endl;
    return 0;
}
