#pragma once

#include <memory>
#include <atomic>

template <typename T>
class rcu_ptr {

    std::shared_ptr<const T> sp;

public:
    // TODO add
    // template <typename Y>
    // rcu_ptr(const std::shared_ptr<Y>& r) {}

    rcu_ptr() = default;
    ~rcu_ptr() = default;

    rcu_ptr(const rcu_ptr& rhs) = delete;
    rcu_ptr& operator=(const rcu_ptr& rhs) = delete;

    rcu_ptr(rcu_ptr&&) = delete;
    rcu_ptr& operator=(rcu_ptr&&) = delete;

    rcu_ptr(const std::shared_ptr<const T>& sp_) : sp(sp_) {}
    rcu_ptr(std::shared_ptr<const T>&& sp_) : sp(std::move(sp_)) {}

    std::shared_ptr<const T> read() const {
        return std::atomic_load_explicit(&sp, std::memory_order_consume);
    }

    // Overwrites the content of the wrapped shared_ptr.
    // We can use it to reset the wrapped data to a new value independent from
    // the old value. ( e.g. vector.clear() )
    void reset(const std::shared_ptr<const T>& r) {
        std::atomic_store_explicit(&sp, r, std::memory_order_release);
    }
    void reset(std::shared_ptr<const T>&& r) {
        std::atomic_store_explicit(&sp, std::move(r),
                                   std::memory_order_release);
    }

    // Updates the content of the wrapped shared_ptr.
    // We can use it to update the wrapped data to a new value which is
    // dependent from the old value ( e.g. vector.push_back() ).
    //
    // @param fun is a lambda which is called whenever an update
    // needs to be done, i.e. it will be called continuously until the update is
    // successful.
    //
    // A call expression with this function is invalid,
    // if T is a non-copyable type.
    template <typename R>
    void copy_update(R&& fun) {
        std::shared_ptr<const T> sp_l =
            std::atomic_load_explicit(&sp, std::memory_order_consume);
        auto exchange_result = false;
        while (!exchange_result) {
            std::shared_ptr<T> r;
            if (sp_l) {
                // deep copy
                r = std::make_shared<T>(*sp_l);
            }

            // update
            std::forward<R>(fun)(r.get());

            // Note, we need to construct a shared_ptr to const,
            // otherwise template type deduction would fail.
            exchange_result = std::atomic_compare_exchange_strong_explicit(
                &sp, &sp_l, std::shared_ptr<const T>(std::move(r)),
                std::memory_order_release, std::memory_order_consume);
        }
    }

#if 0
    // This version requires the client to do the copy with make_shared
    template <typename R>
    void copy_update(R&& fun) {
        std::shared_ptr<T> sp_l = std::atomic_load(&sp);
        auto exchange_result = false;
        while (!exchange_result) {
            auto r = std::forward<R>(fun)(std::shared_ptr<const T>(sp_l));
            exchange_result =
                std::atomic_compare_exchange_strong(&sp, &sp_l, r);
        }
    }
#endif
};
