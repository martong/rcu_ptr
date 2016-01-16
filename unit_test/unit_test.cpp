#include "../versioned_shared_ptr.hpp"
#include <cassert>
#include <iostream>

#define ASSERT(CONDITION)                                                      \
  do                                                                           \
    if (!(CONDITION)) {                                                        \
      printf("Assertion failure %s:%d ASSERT(%s)\n", __FILE__, __LINE__,       \
             #CONDITION);                                                      \
      abort();                                                                 \
    }                                                                          \
  while (0)

struct A {
    int a;
    int b;
    int c;
};

void test() {
    auto p = versioned_shared_ptr<int>{};
    auto new_ = std::make_shared<int>(42);
    p.overwrite(new_);
    ASSERT(42 == *p.read());
}

int main() {
    test();
    std::cout << "OK" << std::endl;
    return 0;
}
