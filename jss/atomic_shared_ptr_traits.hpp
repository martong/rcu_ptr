// atomic_shared_ptr_traits.hpp
//
#pragma once
#include "atomic_shared_ptr"

namespace jss {

template< template <typename> class AtomicSharedPtr >
struct atomic_shared_ptr_traits
{
  template< typename T >
  using atomic_shared_ptr = AtomicSharedPtr<T>;

  template< typename T >
  using shared_ptr = jss::shared_ptr<T>;

  template< typename T, typename... Args >
  static auto make_shared(Args &&...args)
  { return jss::make_shared<T>(std::forward<Args>(args)...); }
};

} // namespace jss

