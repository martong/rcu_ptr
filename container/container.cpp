#include <vector>
#include <numeric>
#include <mutex>
#include <memory>
#include "../rcu_ptr.hpp"
#include "../race_test/ExecuteInLoop.hpp"
#include <thread>
#include <iostream>

#define ASSERT(CONDITION)                                                      \
  do                                                                           \
    if (!(CONDITION)) {                                                        \
      printf("Assertion failure %s:%d ASSERT(%s)\n", __FILE__, __LINE__,       \
             #CONDITION);                                                      \
      abort();                                                                 \
    }                                                                          \
  while (0)


//class X {
    //std::vector<int> v;
    //mutable std::mutex m;
//public:
    //int sum() const { // read operation
        //std::lock_guard<std::mutex> lock{m};
        //return std::accumulate(v.begin(), v.end(), 0);
    //}
    //void add(int i) { // write operation
        //std::lock_guard<std::mutex> lock{m};
        //v.push_back(i);
    //}
//};

// RACE on the pointee:
//class X {
    //std::shared_ptr<std::vector<int>> v;
    //mutable std::mutex m;
//public:
    //X() : v(std::make_shared<std::vector<int>>()) {}
    //int sum() const { // read operation
        //std::shared_ptr<std::vector<int>> local_copy;
        //{
            //std::lock_guard<std::mutex> lock{m};
            //local_copy = v;
        //}
        //return std::accumulate(local_copy->begin(), local_copy->end(), 0);
    //}
    //void add(int i) { // write operation
        //std::shared_ptr<std::vector<int>> local_copy;
        //{
            //std::lock_guard<std::mutex> lock{m};
            //local_copy = v;
        //}
        //local_copy->push_back(i);
        //{
            //std::lock_guard<std::mutex> lock{m};
            //v = local_copy;
        //}
    //}
//};

// if there are two concurrent write operations than we might miss one update,
// assert will fail
//class X {
    //std::shared_ptr<std::vector<int>> v;
    //mutable std::mutex m;
//public:
    //X() : v(std::make_shared<std::vector<int>>()) {}
    //int sum() const { // read operation
        //std::shared_ptr<std::vector<int>> local_copy;
        //{
            //std::lock_guard<std::mutex> lock{m};
            //local_copy = v;
        //}
        //return std::accumulate(local_copy->begin(), local_copy->end(), 0);
    //}
    //void add(int i) { // write operation
        //std::shared_ptr<std::vector<int>> local_copy;
        //{
            //std::lock_guard<std::mutex> lock{m};
            //local_copy = v;
        //}
        //auto local_deep_copy = std::make_shared<std::vector<int>>(*local_copy);
        //local_deep_copy->push_back(i);
        //{
            //std::lock_guard<std::mutex> lock{m};
            //v = local_deep_copy;
        //}
    //}
//};

//class X {
    //std::shared_ptr<std::vector<int>> v;
//public:
    //X() : v(std::make_shared<std::vector<int>>()) {}
    //int sum() const { // read operation
        //auto local_copy = std::atomic_load(&v);
        //return std::accumulate(local_copy->begin(), local_copy->end(), 0);
    //}
    //void add(int i) { // write operation
        //auto local_copy = std::atomic_load(&v);
        //auto exchange_result = false;
        //while (!exchange_result) {
            //// we need a deep copy
            //auto local_deep_copy = std::make_shared<std::vector<int>>(
                    //*local_copy);
            //local_deep_copy->push_back(i);
            //exchange_result =
                //std::atomic_compare_exchange_strong(&v, &local_copy,
                                                    //local_deep_copy);
        //}
    //}
//};

class X {
    rcu_ptr<std::vector<int>> v;
public:
    X() { v.overwrite(std::make_shared<std::vector<int>>()); }
    int sum() const { // read operation
        std::shared_ptr<const std::vector<int>> local_copy = v.read();
        return std::accumulate(local_copy->begin(), local_copy->end(), 0);
    }
    void add(int i) { // write operation
        v.update([i](const std::vector<int>& v) {
            auto new_ = v;
            new_.push_back(i);
            return new_;
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
    test_sum_add();
}
