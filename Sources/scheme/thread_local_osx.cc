#include "scheme/thread_local.h"
#ifdef OS_IS_OSX

namespace scm {
class pthread_error {
  DEFINE_DEFAULT_COPYABLE_TYPE(pthread_error);

 private:
  int value_;

 public:
  pthread_error(const int value = PTHREAD_OK) :
    value_(value) {}
  ~pthread_error() = default;

  auto value() const -> int {
    return value_;
  }

  auto IsOk() const -> bool {
    return value() == PTHREAD_OK;
  }

  operator bool() const {
    return IsOk();
  }

  friend auto operator<<(std::ostream& stream, const pthread_error& rhs) -> std::ostream& {
    return stream << (rhs ? "Ok" : strerror(rhs.value()));
  }
};

auto InitThreadLocal(ThreadLocalKey& key, const uword init_value) -> bool {
  const pthread_error status = pthread_key_create(&key, nullptr);
  LOG_IF(ERROR, !status) << "failed to initialize ThreadLocal: " << status;
  if (init_value != UNALLOCATED)
    return SetThreadLocal(key, init_value);
  return status;
}

auto SetThreadLocal(const ThreadLocalKey& key, const uword value) -> bool {
  const auto ptr = (const void*)value;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  const pthread_error status = pthread_setspecific(key, ptr);
  LOG_IF(ERROR, !status) << "failed to set ThreadLocal to `" << ptr << "`: " << status;
  return status;
}

auto GetThreadLocal(const ThreadLocalKey& key) -> uword {
  const auto ptr = pthread_getspecific(key);
  return ptr ? ((uword)ptr) : UNALLOCATED;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}
}  // namespace scm

#endif  // OS_IS_OSX