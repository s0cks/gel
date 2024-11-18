#ifndef SCM_OS_THREAD_H
#define SCM_OS_THREAD_H

#include <string>

#include "scheme/common.h"
#ifdef OS_IS_LINUX
#include "scheme/os_thread_linux.h"
#elif OS_IS_OSX
#include "scheme/os_thread_osx.h"
#elif OS_IS_WINDOWS
#include "scheme/os_thread_windows.h"
#endif

namespace scm {
ThreadId GetCurrentThreadId();
std::string GetThreadName(const ThreadId& thread);
bool SetThreadName(const ThreadId& thread, const std::string& name);
bool InitializeThreadLocal(ThreadLocalKey& key);
bool SetCurrentThreadLocal(const ThreadLocalKey& key, const void* value);
void* GetCurrentThreadLocal(const ThreadLocalKey& key);
bool Start(ThreadId* thread, const std::string& name, const ThreadHandler& func, void* data);
bool Join(const ThreadId& thread);
bool Compare(const ThreadId& lhs, const ThreadId& rhs);
int GetCurrentThreadCount();

static inline std::string GetCurrentThreadName() {
  return GetThreadName(GetCurrentThreadId());
}

static inline bool SetCurrentThreadName(const std::string& name) {
  return SetThreadName(GetCurrentThreadId(), name);
}

template <typename T>
class ThreadLocal {
  DEFINE_NON_COPYABLE_TYPE(ThreadLocal<T>);

 private:
  ThreadLocalKey key_{};

 public:
  ThreadLocal() {
    LOG_IF(FATAL, !InitializeThreadLocal(key_));
  }
  virtual ~ThreadLocal() = default;

  auto GetKey() const -> const ThreadLocalKey& {
    return key_;
  }

  virtual auto Set(T* value) const -> bool {
    return SetCurrentThreadLocal(GetKey(), (void*)value);
  }

  virtual auto Get() const -> T* {
    return (T*)GetCurrentThreadLocal(GetKey());
  }

  auto Has() const -> bool {
    return Get() != nullptr;
  }

  inline auto IsEmpty() const -> bool {
    return Get() == nullptr;
  }

  operator bool() const {
    return Has();
  }

  auto operator=(T* value) -> ThreadLocal<T>& {
    PRT_ASSERT(value);
    Set(value);
    return *this;
  }

  friend auto operator<<(std::ostream& stream, const ThreadLocal<T>& rhs) -> std::ostream& {
    return stream << *rhs.Get();
  }
};

template <typename T>
class LazyThreadLocal : public ThreadLocal<T> {
  DEFINE_NON_COPYABLE_TYPE(LazyThreadLocal<T>);

 public:
  using Supplier = std::function<T*()>;

 private:
  static inline auto CreateDefaultSupplier() -> Supplier {
    return []() {
      return new T();
    };
  }

 private:
  Supplier supplier_;

  inline auto Supply() const -> T* {
    return supplier_();
  }

 public:
  explicit LazyThreadLocal(const Supplier& supplier = CreateDefaultSupplier()) :
    supplier_(supplier) {}
  virtual ~LazyThreadLocal() = default;

  auto GetSupplier() const -> const Supplier& {
    return supplier_;
  }

  auto Get() const -> T* override {
    const auto value = ThreadLocal<T>::Get();
    if (value)
      return value;
    const auto supplied = Supply();
    LOG_IF(FATAL, !supplied) << "failed to supply value for ThreadLocal.";
    LOG_IF(FATAL, !ThreadLocal<T>::Set(supplied)) << "failed to set ThreadLocal value.";
    return supplied;
  }
};
}  // namespace scm

#endif  // SCM_OS_THREAD_H
