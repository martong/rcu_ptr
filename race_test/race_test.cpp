#include "../versioned_shared_ptr.hpp"
#include "ExecuteInLoop.hpp"
#include <thread>

void race() {
    int a;

    std::thread t1{[&a]() { executeInLoop<1000>([&a]() { ++a; }); }};
    executeInLoop<1000>([&a]() { --a; });

    t1.join();
}

void test_read_write() {
    auto p = versioned_shared_ptr<int>{};

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            auto new_ = std::make_shared<int>(42);
            p.write(new_);
        });
    }};

    executeInLoop<10000>([&p]() { auto& x = *p.read(); });

    t1.join();
}

void test_write_write() {
    auto p = versioned_shared_ptr<int>{};

    auto l = [&p]() {
        executeInLoop<10000>([&p]() {
            auto new_ = std::make_shared<int>(42);
            p.write(new_);
        });
    };

    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();
}

void test_read_read() {
    auto p = versioned_shared_ptr<int>{};
    p.write(std::make_shared<int>(42));

    auto l = [&p]() { executeInLoop<10000>([&p]() { auto& x = *p.read(); }); };
    std::thread t1{l};
    std::thread t2{l};

    t1.join();
    t2.join();
}

void test_assign_write() {
    auto p = versioned_shared_ptr<int>{};

    std::thread t2{[&p]() {
        executeInLoop<10000>([&p]() {
			p = versioned_shared_ptr<int>{};
        });
    }};

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            auto new_ = std::make_shared<int>(42);
            p.write(new_);
        });
    }};

    t1.join();
    t2.join();
}

void test_copyctor_write() {
    auto p = versioned_shared_ptr<int>{};

    std::thread t2{[&p]() {
        executeInLoop<10000>([&p]() {
			auto p2 = p;
        });
    }};

    std::thread t1{[&p]() {
        executeInLoop<10000>([&p]() {
            auto new_ = std::make_shared<int>(42);
            p.write(new_);
        });
    }};

    t1.join();
    t2.join();
}

int main() {
    test_read_write();
    test_write_write();
    test_read_read();
	test_assign_write();
	test_copyctor_write();
}
