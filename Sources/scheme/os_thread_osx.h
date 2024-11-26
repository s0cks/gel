#ifndef SCM_OS_THREAD_OSX_H
#define SCM_OS_THREAD_OSX_H

#include "scheme/common.h"
#ifndef SCM_OS_THREAD_H
#error "Please #include <scheme/os_thread.h> instead of <scheme/os_thread_osx.h>"
#endif  // SCM_OS_THREAD_H

#include <pthread.h>

namespace scm {
static const int kThreadNameMaxLength = 16;
static const int kThreadMaxResultLength = 128;

using ThreadLocalKey = pthread_key_t;
using ThreadId = pthread_t;
using ThreadHandler = void (*)(void*);

#ifndef PTHREAD_OK
#define PTHREAD_OK 0
#endif  // PTHREAD_OK

class pthread_status {
  DEFINE_DEFAULT_COPYABLE_TYPE(pthread_status);

 private:
  int value_;

 public:
  constexpr pthread_status(const int value) :
    value_(value) {}
  ~pthread_status() = default;

  constexpr auto value() const -> int {
    return value_;
  }

  constexpr auto ok() const -> bool {
    return value() == PTHREAD_OK;
  }

  constexpr operator int() const {
    return value();
  }

  constexpr operator bool() const {
    return ok();
  }

  friend auto operator<<(std::ostream& stream, const pthread_status& rhs) -> std::ostream& {
    return stream << (rhs ? "Ok" : strerror(rhs.value()));
  }
};

}  // namespace scm

#endif  // SCM_OS_THREAD_OSX_H
