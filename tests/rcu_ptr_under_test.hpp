#pragma once

#include <rcu_ptr.hpp>

#ifdef TEST_WITH_JSS_ASP

#include <jss/atomic_shared_ptr>
#include <jss/atomic_shared_ptr_traits.hpp>

using asp_traits = jss::atomic_shared_ptr_traits<jss::atomic_shared_ptr>;

template <typename T>
using rcu_ptr_under_test = rcu_ptr<T, jss::atomic_shared_ptr, asp_traits>;

#else

using asp_traits =
    detail::atomic_shared_ptr_traits<detail::__std::atomic_shared_ptr>;

template <typename T>
using rcu_ptr_under_test = rcu_ptr<T>;

#endif

