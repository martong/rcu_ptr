// scoped_thread.hpp
//
#pragma once

namespace tools {

template< typename Thread >
class scoped_thread {
public:
  using thread_type = Thread;

  scoped_thread() = default;
  ~scoped_thread() noexcept
  { Join(Thr); }

  template< typename F >
  scoped_thread(F &&Fn)
  : Thr(std::forward<F>(Fn))
  {}

  scoped_thread(thread_type const&) = delete;
  scoped_thread(scoped_thread const&) = delete;
  scoped_thread(scoped_thread &&Other) noexcept
  : scoped_thread(std::move(Other.Thr))
  {}

  scoped_thread(thread_type &&Thrd) noexcept
  : Thr(std::move(Thrd))
  {}

  scoped_thread &operator= (thread_type const&) = delete;
  scoped_thread &operator= (scoped_thread const&) = delete;
  scoped_thread &operator= (scoped_thread &&Rhs)
  { return( *this = std::move(Rhs.Thr) ); }

  scoped_thread &operator= (thread_type &&Thrd)
  {
    if( &Thrd != &Thr )
    {
      Join(Thr);
      Thr = std::move(Thrd);
    }
    return *this;
  }

private:
  static void Join(thread_type &Thrd) noexcept
  { if (Thrd.joinable()) Thrd.join(); }

  thread_type Thr;
};

} // namespace tools

