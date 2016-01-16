#include "../versioned_shared_ptr.hpp"
#include "ExecuteInLoop.hpp"
#include <thread>

#define ASSERT(CONDITION)                                                      \
    do                                                                         \
        if (!(CONDITION)) {                                                    \
            printf("Assertion failure %s:%d ASSERT(%s)\n", __FILE__, __LINE__, \
                   #CONDITION);                                                \
            abort();                                                           \
        }                                                                      \
    while (0)

void race() {
    int a;

    std::thread t1{[&a]() { executeInLoop<1000>([&a]() { ++a; }); }};
    executeInLoop<1000>([&a]() { --a; });

    t1.join();
}

void test_read_overwrite() {
    auto p = versioned_shared_ptr<int>{};

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            auto new_ = std::make_shared<int>(42);
            p.overwrite(new_);
        });
    }};

    executeInLoop<10000>([&p]() { auto& x = *p.read(); });

    t1.join();
}

void test_read_update() {
    auto p = versioned_shared_ptr<int>{};

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            p.update([](int) {
                return 42;
            });
        });
    }};

    executeInLoop<10000>([&p]() { auto& x = *p.read(); });

    t1.join();
    ASSERT(*p.read() == 42);
}

void test_overwrite_overwrite() {
    auto p = versioned_shared_ptr<int>{};

    auto l = [&p]() {
        executeInLoop<10000>([&p]() {
            auto new_ = std::make_shared<int>(42);
            p.overwrite(new_);
        });
    };

    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();
}

//void test_read_update() {
    //auto p = versioned_shared_ptr<int>{};

    //std::thread t1{[&p]() {
        //executeInLoop<10000>([&p]() {
            //p.update([](const std::shared_ptr<const int>& v) {
                //auto v2 = std::make_shared<int>(42);
                //return v2;
            //});
        //});
    //}};

    //executeInLoop<10000>([&p]() { auto& x = *p.read(); });

    //t1.join();
    //ASSERT(*p.read() == 42);
//}

//void test_update_update() {
    //auto p = versioned_shared_ptr<int>{};
    //p.update([](const std::shared_ptr<const int>& v) {
        //return std::make_shared<int>(0);
    //});

    //std::thread t1{[&p]() {
        //executeInLoop<10000>([&p]() {
            //p.update([](const std::shared_ptr<const int>& v) {
                //return std::make_shared<int>((*v) + 1);
            //});
        //});
    //}};

    //std::thread t2{[&p]() {
        //executeInLoop<10000>([&p]() {
            //p.update([](const std::shared_ptr<const int>& v) {
                //return std::make_shared<int>((*v) + 1);
            //});
        //});
    //}};

    //t1.join();
    //t2.join();
    //ASSERT(*p.read() == 20000);
//}

void test_update_update() {
    auto p = versioned_shared_ptr<int>{};
    p.update([](const int&) {
        return 0;
    });

    auto l = [&p]() {
        executeInLoop<10000>([&p]() {
            p.update([](int v) {
                return ++v;
            });
        });
    };

    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();
    ASSERT(*p.read() == 20000);
}

void test_update_push_back() {
    const int i = 2;
    using V = std::vector<int>;
    auto p = versioned_shared_ptr<V>{};
    p.update([](const V&) {
        return V{};
    });

    auto l = [&p, &i]() {
        executeInLoop<1000>([&p, &i]() {
            p.update([&i](const V& v) {
                auto new_ = v;
                new_.push_back(i);
                return new_;
            });
        });
    };

    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();
    ASSERT(p.read()->size() == 2000);
}

void test_read_read() {
    auto p = versioned_shared_ptr<int>{};
    p.overwrite(std::make_shared<int>(42));

    auto l = [&p]() { executeInLoop<10000>([&p]() { auto& x = *p.read(); }); };
    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();
}

void test_assign_overwrite() {
    auto p = versioned_shared_ptr<int>{};

    std::thread t2{[&p]() {
        executeInLoop<10000>([&p]() { p = versioned_shared_ptr<int>{}; });
    }};

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            auto new_ = std::make_shared<int>(42);
            p.overwrite(new_);
        });
    }};

    t1.join();
    t2.join();
}

void test_copyctor_overwrite() {
    auto p = versioned_shared_ptr<int>{};

    std::thread t2{[&p]() { executeInLoop<10000>([&p]() { auto p2 = p; }); }};

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            auto new_ = std::make_shared<int>(42);
            p.overwrite(new_);
        });
    }};

    t1.join();
    t2.join();
}

void test_copyctor_assign() {
    auto p = versioned_shared_ptr<int>{};

    std::thread t1{[&p]() { executeInLoop<10000>([&p]() { auto p2 = p; }); }};

    std::thread t2{[&p]() {
        executeInLoop<10000>([&p]() { p = versioned_shared_ptr<int>{}; });
    }};

    t1.join();
    t2.join();
}

int main() {
    test_read_overwrite();
    test_read_update();
    test_overwrite_overwrite();
    test_update_update();
    test_update_push_back();
    test_read_read();
    test_assign_overwrite();
    test_copyctor_overwrite();
    test_copyctor_assign();
}
