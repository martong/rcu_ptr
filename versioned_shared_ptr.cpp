#include <memory>
#include <atomic>

#include <cassert>
#include <iostream>

template <typename T>
class versioned_shared_ptr {

    std::shared_ptr<T> sp;

public:
    // template <typename Y>
    // versioned_shared_ptr(const std::shared_ptr<Y>& r) {}

    std::shared_ptr<const T> read() { return std::atomic_load(&sp); }

    void write(const std::shared_ptr<T>& r) {
        auto sp_l = std::atomic_load(&sp);
        auto exchange_result = false;
        //while (sp_l && !exchange_result) {
        while (!exchange_result) {
            // True if exchange was performed
            // If sp == sp_l (share ownership of the same pointer),
            //   assigns r into sp
            // If sp != sp_l, assigns sp into sp_l
            exchange_result =
                std::atomic_compare_exchange_strong(&sp, &sp_l, r);
        }
    }
};

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
    p.write(new_);
    //auto rp = p.read();
    //ASSERT(42 == *rp);
    ASSERT(42 == *p.read());
}

int main() {
    test();
    std::cout << "OK" << std::endl;
    return 0;
}
