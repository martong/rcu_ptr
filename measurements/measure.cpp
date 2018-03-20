#include <atomic>
#include <cassert>
#include <iostream>
#include <mutex>
#include <numeric>
#include <tests/rcu_ptr_under_test.hpp>
#include <thread>
#include <vector>

#include <tbb/queuing_rw_mutex.h>
#include <tbb/spin_rw_mutex.h>

#include <urcu-pointer.h>
#ifdef X_URCU_BP
  #include <urcu-bp.h>
#else
  #include <urcu.h>
#endif

class XRcuPtr {
    rcu_ptr_under_test<std::vector<int>> v;
    const int default_value = 1;

public:
    XRcuPtr(size_t vec_size)
        : v(asp_traits::make_shared<std::vector<int>>(vec_size,
                                                      default_value)) {}

    int read_one(unsigned index) const {
        asp_traits::shared_ptr<const std::vector<int>> local_copy = v.read();
        assert(index < local_copy->size());
        return (*local_copy)[index];
    }
    int read_all() const { // sum
        asp_traits::shared_ptr<const std::vector<int>> local_copy = v.read();
        return std::accumulate(local_copy->begin(), local_copy->end(), 0);
    }

    void update_all(int value) {
        v.copy_update([=](std::vector<int>* copy) {
            for (auto& e : *copy) {
                e = value;
            }
        });
    }
};

class XStdMutex {
    std::vector<int> v;
    const int default_value = 1;
    mutable std::mutex m;

public:
    XStdMutex(size_t vec_size) : v(vec_size, default_value) {}

    int read_one(unsigned index) const {
        std::lock_guard<std::mutex> lock{m};
        assert(index < v.size());
        return v[index];
    }
    int read_all() const { // sum
        std::lock_guard<std::mutex> lock{m};
        return std::accumulate(v.begin(), v.end(), 0);
    }

    void update_all(int value) {
        std::lock_guard<std::mutex> lock{m};
        for (auto& e : v) {
            e = value;
        }
    }
};

class XTbbQueuingRwMutex {
    std::vector<int> v;
    const int default_value = 1;
    mutable tbb::queuing_rw_mutex m;

public:
    XTbbQueuingRwMutex(size_t vec_size) : v(vec_size, default_value) {}

    int read_one(unsigned index) const {
        tbb::queuing_rw_mutex::scoped_lock lock{m, false}; // read lock
        assert(index < v.size());
        return v[index];
    }
    int read_all() const {                                 // sum
        tbb::queuing_rw_mutex::scoped_lock lock{m, false}; // read lock
        return std::accumulate(v.begin(), v.end(), 0);
    }

    void update_all(int value) {
        tbb::queuing_rw_mutex::scoped_lock lock{m}; // write lock
        for (auto& e : v) {
            e = value;
        }
    }
};

class XTbbSpinRwMutex {
    std::vector<int> v;
    const int default_value = 1;
    mutable tbb::spin_rw_mutex m;

public:
    XTbbSpinRwMutex(size_t vec_size) : v(vec_size, default_value) {}

    int read_one(unsigned index) const {
        tbb::spin_rw_mutex::scoped_lock lock{m, false}; // read lock
        assert(index < v.size());
        return v[index];
    }
    int read_all() const {                              // sum
        tbb::spin_rw_mutex::scoped_lock lock{m, false}; // read lock
        return std::accumulate(v.begin(), v.end(), 0);
    }

    void update_all(int value) {
        tbb::spin_rw_mutex::scoped_lock lock{m}; // write lock
        for (auto& e : v) {
            e = value;
        }
    }
};

class XURCU {
    std::vector<int>* v;
    const int default_value = 1;
    mutable std::mutex m; // to support concurrent writers

public:
    XURCU(size_t vec_size) : v(new std::vector<int>(vec_size, default_value)) {}

    int read_one(unsigned index) const {
        rcu_read_lock();
        const std::vector<int>* local_copy = rcu_dereference(v);
        assert(index < local_copy->size());
        int result = (*local_copy)[index];
        rcu_read_unlock();
        return result;
    }
    int read_all() const { // sum
        rcu_read_lock();
        const std::vector<int>* local_copy = rcu_dereference(v);
        int result = std::accumulate(local_copy->begin(), local_copy->end(), 0);
        rcu_read_unlock();
        return result;
    }

    void update_all(int value) {
        std::lock_guard<std::mutex> lock{m}; // support concurrent writers

        rcu_read_lock();
        std::vector<int>* local_copy = rcu_dereference(v);
        std::vector<int>* local_deep_copy = new std::vector<int>(*local_copy);
        rcu_read_unlock();

        assert(index < copy->size());
        for (auto& e : *local_deep_copy) {
            e = value;
        }
        synchronize_rcu();
        delete local_copy;
        rcu_assign_pointer(v, local_deep_copy);
    }

    ~XURCU() {
        synchronize_rcu();
        delete v;
    }
};

class RoundRobin {
    long long i = 0;
    unsigned size;

public:
    RoundRobin(unsigned size) : size(size) {}
    unsigned next() { return i++ % size; }
};

template <typename X>
struct Driver {
    X x;
    std::atomic<bool> stop = ATOMIC_FLAG_INIT;
    unsigned vec_size, num_readers, num_writers;
    std::mutex finish_mtx;
    std::vector<long long> reader_cycles;
    std::vector<long long> writer_cycles;

    Driver(unsigned vec_size, unsigned num_readers, unsigned num_writers)
        : x(vec_size),
          vec_size(vec_size),
          num_readers(num_readers),
          num_writers(num_writers) {}

    void timer_fun() {
        using namespace std::chrono_literals;
        auto start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(3s);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        stop.store(true, std::memory_order_relaxed);
        std::cout << "Waited " << elapsed.count() << " ms\n";
    }

    // void reader_fun_rr() {
    // RoundRobin rr{vec_size};
    // x.read_one(rr.next());
    //}

    // void reader_fun_one(unsigned index) { x.read_one(index); }

    void reader_fun() {
        long long cycles = 0;
        while (!stop.load(std::memory_order_relaxed)) {
            for (int i = 0; i < 1000; ++i) {
                x.read_all();
                ++cycles;
            }
        }
        {
            std::lock_guard<std::mutex> lock(finish_mtx);
            reader_cycles.push_back(cycles);
        }
    }

    void one_reader_fun() {
        long long cycles = 0;
        RoundRobin rr{vec_size};
        while (!stop.load(std::memory_order_relaxed)) {
            for (int i = 0; i < 1000; ++i) {
                x.read_one(rr.next());
                ++cycles;
            }
        }
        {
            std::lock_guard<std::mutex> lock(finish_mtx);
            reader_cycles.push_back(cycles);
        }
    }

    void writer_fun() {
        long long cycles = 0;
        while (!stop.load(std::memory_order_relaxed)) {
            for (int i = 0; i < 1000; ++i) {
                x.update_all(i);
                ++cycles;
            }
        }
        {
            std::lock_guard<std::mutex> lock(finish_mtx);
            writer_cycles.push_back(cycles);
        }
    }

    void print_stats() {
        long long reader_sum = 0, writer_sum = 0;
        for (auto cycles : reader_cycles) {
            reader_sum += cycles;
            std::cout << "reader thread: " << cycles << "\n";
        }
        for (auto cycles : writer_cycles) {
            writer_sum += cycles;
            std::cout << "writer thread: " << cycles << "\n";
        }
        std::cout << "reader sum: " << reader_sum << "\n";
        std::cout << "writer sum: " << writer_sum << "\n";
        std::cout << "reader av: " << (reader_cycles.size() > 0
                                           ? reader_sum / reader_cycles.size()
                                           : 0)
                  << "\n";
        std::cout << "writer av: " << (writer_cycles.size() > 0
                                           ? writer_sum / writer_cycles.size()
                                           : 0)
                  << "\n";
    }
};

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Wrong program args!\n";
        exit(-1);
    }
    if (std::string(argv[1]) != "read_one" && std::string(argv[1]) != "read_all") {
        std::cerr << "Wrong program args of read one/all!\n";
        exit(-2);
    }
    bool read_one = std::string(argv[1]) == "read_one";
    if (read_one) {
        std::cout << "read_one set\n";
    }
    unsigned vec_size = atoi(argv[2]);
    assert(vec_size >= 1);
    unsigned num_readers = atoi(argv[3]);
    unsigned num_writers = atoi(argv[4]);

#ifdef X_STD_MUTEX
    Driver<XStdMutex> driver{vec_size, num_readers, num_writers};
#elif defined X_TBB_QRW_MUTEX
    Driver<XTbbQueuingRwMutex> driver{vec_size, num_readers, num_writers};
#elif defined X_TBB_SRW_MUTEX
    Driver<XTbbSpinRwMutex> driver{vec_size, num_readers, num_writers};
#elif defined X_URCU
    Driver<XURCU> driver{vec_size, num_readers, num_writers};
#else
    Driver<XRcuPtr> driver{vec_size, num_readers, num_writers};
#endif

    std::thread timer_thread([&driver]() { driver.timer_fun(); });
    std::vector<std::thread> reader_threads;
    std::vector<std::thread> writer_threads;

    for (unsigned i = 0; i < num_readers; ++i) {
        if (read_one) {
            reader_threads.push_back(
                std::thread([&driver]() { driver.one_reader_fun(); }));
        } else {
            reader_threads.push_back(
                std::thread([&driver]() { driver.reader_fun(); }));
        }
    }
    for (unsigned i = 0; i < num_writers; ++i) {
        writer_threads.push_back(
            std::thread([&driver]() { driver.writer_fun(); }));
    }

    timer_thread.join();
    for (auto& t : reader_threads) {
        t.join();
    }
    for (auto& t : writer_threads) {
        t.join();
    }
    driver.print_stats();

    return 0;
}
