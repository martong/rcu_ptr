#include "../rcu_ptr.hpp"
#include "ExecuteInLoop.hpp"
#include <thread>
#include <iostream>

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
    auto p = make_rcu_ptr<int>(42);

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            auto new_ = std::make_shared<int>(42);
            p.overwrite(new_);
        });
    }};

    executeInLoop<10000>([&p]() { auto& x = *p.read(); });

    t1.join();
}

void test_read_copy_update() {
    auto p = make_rcu_ptr<int>(42);

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            p.copy_update([](auto cp) {
                *cp = 42;
            });
        });
    }};

    executeInLoop<10000>([&p]() { auto& x = *p.read(); });

    t1.join();
    ASSERT(*p.read() == 42);
}

void test_overwrite_overwrite() {
    auto p = rcu_ptr<int>{};

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
    //auto p = rcu_ptr<int>{};

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
    //auto p = rcu_ptr<int>{};
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

void test_copy_update_copy_update() {
    auto p = make_rcu_ptr<int>(0);

    auto l = [&p]() {
        executeInLoop<10000>([&p]() {
            p.copy_update([](auto copy) {
                (*copy)++;
            });
        });
    };

    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();
    ASSERT(*p.read() == 20000);
}

void test_copy_update_push_back() {
    using V = std::vector<int>;
    auto p = make_rcu_ptr<V>();
    const int i = 2;

    auto l = [&p, &i]() {
        executeInLoop<1000>([&p, &i]() {
            p.copy_update([&i](auto copy) {
                copy->push_back(i);
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
    auto p = make_rcu_ptr<int>(42);

    auto l = [&p]() { executeInLoop<10000>([&p]() { auto& x = *p.read(); }); };
    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();
}

void test_assign_overwrite() {
    auto p = rcu_ptr<int>{};

    std::thread t2{[&p]() {
        executeInLoop<10000>([&p]() { p = rcu_ptr<int>{}; });
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
    auto p = rcu_ptr<int>{};

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
    auto p = rcu_ptr<int>{};

    std::thread t1{[&p]() { executeInLoop<10000>([&p]() { auto p2 = p; }); }};

    std::thread t2{[&p]() {
        executeInLoop<10000>([&p]() { p = rcu_ptr<int>{}; });
    }};

    t1.join();
    t2.join();
}

class X {
    rcu_ptr<std::vector<int>> v;
public:
    X() { v.overwrite(std::make_shared<std::vector<int>>()); }
    int sum() const { // read operation
        std::shared_ptr<const std::vector<int>> local_copy = v.read();
        return std::accumulate(local_copy->begin(), local_copy->end(), 0);
    }
    void add(int i) { // write operation
        v.copy_update([i](auto copy) {
            copy->push_back(i);
        });
    }
};

void test_sum_add() {
    X x{};

    int sum = 0;
    std::thread t2{[&x, &sum]() {
        executeInLoop<1000>([&x, &sum]() {
            sum = x.sum();
        });
    }};

    std::thread t1{[&x]() {
        executeInLoop<1000>([&x]() {
            x.add(3);
        });
    }};

    std::thread t3{[&x]() {
        executeInLoop<1000>([&x]() {
            x.add(4);
        });
    }};

    t1.join();
    t2.join();
    t3.join();
    std::cout << x.sum() << std::endl;
    ASSERT(x.sum() == 7000);
}

int main() {
    test_read_overwrite();
    test_read_copy_update();
    test_overwrite_overwrite();
    test_copy_update_copy_update();
    test_copy_update_push_back();
    test_read_read();
    test_assign_overwrite();
    test_copyctor_overwrite();
    test_copyctor_assign();
    test_sum_add();
}
