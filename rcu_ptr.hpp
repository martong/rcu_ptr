#pragma once

#include <detail/atomic_shared_ptr_traits.hpp>
#include <detail/atomic_shared_ptr.hpp>
#include <memory>
#include <atomic>

template< 
  typename T
, template <typename> class AtomicSharedPtr = detail::__std::atomic_shared_ptr
, typename ASPTraits = detail::atomic_shared_ptr_traits<AtomicSharedPtr>  >
class rcu_ptr {

    template< typename _T >
    using atomic_shared_ptr = typename ASPTraits::template atomic_shared_ptr<_T>;

    atomic_shared_ptr<T> asp;

public:
    template< typename _T >
    using shared_ptr   = typename ASPTraits::template shared_ptr<_T>;
    using element_type = typename shared_ptr<T>::element_type;

    // TODO add
    // template <typename Y>
    // rcu_ptr(const std::shared_ptr<Y>& r) {}

    rcu_ptr() = default;

    rcu_ptr(const shared_ptr<T>& desired) 
    : asp(desired)
    {}

    rcu_ptr(shared_ptr<T>&& desired)
    : asp(std::move(desired))
    {}

    rcu_ptr(const rcu_ptr&) = delete;
    rcu_ptr &operator= (const rcu_ptr&) = delete;
    rcu_ptr(rcu_ptr&&) = delete;
    rcu_ptr& operator=(rcu_ptr&&) = delete;

    ~rcu_ptr() = default;

    void operator= (const shared_ptr<T>& desired) 
    { reset(desired); }

    // Move
    // Move operations are not generated since we delete the copy operations.
    // However, the syntax like
    //     auto p = rcu_ptr<T>{};
    // should be supported, therefore delete the move operations explicitly
    // is not an option.
    //     rcu_ptr(rcu_ptr&&) = delete;
    //     rcu_ptr& operator=(rcu_ptr&&) = delete;

    shared_ptr<const T> read() const {
        return asp.load(std::memory_order_consume);
    }

    // Overwrites the content of the wrapped shared_ptr.
    // We can use it to reset the wrapped data to a new value independent from
    // the old value. ( e.g. vector.clear() )
    void reset(const shared_ptr<T>& r) {
        asp.store(r, std::memory_order_release);
    }

    void reset(shared_ptr<T>&& r) {
        asp.store(std::move(r), std::memory_order_release);
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
        shared_ptr<T> sp_l = asp.load(std::memory_order_consume);
        shared_ptr<T> r;
        do {
            if (sp_l) {
                // deep copy
                r = ASPTraits::template make_shared<T>(*sp_l);
            }

            // update
            std::forward<R>(fun)(r.get());
        }
        while (! asp.compare_exchange_strong( sp_l, std::move(r),
                                              std::memory_order_release,
                                              std::memory_order_consume ));
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
                asp.compare_exchange_strong(&sp_l, r);
        }
    }
#endif
};

