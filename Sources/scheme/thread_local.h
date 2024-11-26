#ifndef SCM_THREAD_LOCAL_H
#define SCM_THREAD_LOCAL_H

#include "scheme/common.h"
#include "scheme/os_thread.h"
#include "scheme/platform.h"

namespace scm {
auto InitThreadLocal(ThreadLocalKey& local, const uword init_value = UNALLOCATED) -> bool;
auto SetThreadLocal(const ThreadLocalKey& local, const uword value) -> bool;
auto GetThreadLocal(const ThreadLocalKey& local) -> uword;

class ThreadLocalBase {
  DEFINE_NON_COPYABLE_TYPE(ThreadLocalBase);

 private:
  ThreadLocalKey local_{};

 protected:
  explicit ThreadLocalBase(const uword init_value = UNALLOCATED) {
    InitThreadLocal(local_, init_value);
  }

  auto SetAddress(const uword value) const -> bool {
    return SetThreadLocal(GetLocal(), value);
  }

 public:
  virtual ~ThreadLocalBase() = default;

  auto GetLocal() const -> const ThreadLocalKey& {
    return local_;
  }

  auto GetAddress() const -> uword {
    return GetThreadLocal(GetLocal());
  }

  auto Has() const -> bool {
    return GetAddress() != UNALLOCATED;
  }

  inline auto IsEmpty() const -> bool {
    return GetAddress() == UNALLOCATED;
  }

  auto operator=(const uword rhs) -> ThreadLocalBase& {
    ASSERT(rhs >= UNALLOCATED);
    SetAddress(rhs);
    return *this;
  }
};

template <typename T>
class ThreadLocal : public ThreadLocalBase {
  DEFINE_NON_COPYABLE_TYPE(ThreadLocal<T>);

 public:
  explicit ThreadLocal(T* init_value = UNALLOCATED) :
    ThreadLocalBase((uword)init_value) {}
  virtual ~ThreadLocal() = default;

  virtual auto Set(const T* value) const -> bool {
    return SetAddress((uword)value);
  }

  virtual auto Get() const -> T* {
    return (T*)GetAddress();
  }

  operator bool() const {
    return Has();
  }

  auto operator=(const T* value) -> ThreadLocal<T>& {
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
    if (!ThreadLocalBase::IsEmpty())
      return ThreadLocal<T>::Get();
    const auto supplied = Supply();
    LOG_IF(FATAL, !supplied) << "failed to supply value for ThreadLocal.";
    LOG_IF(FATAL, !ThreadLocal<T>::Set(supplied)) << "failed to set ThreadLocal value.";
    return supplied;
  }

  operator bool() const {
    return !ThreadLocalBase::IsEmpty();
  }

  auto operator=(const T* value) -> LazyThreadLocal<T>& {
    PRT_ASSERT(value);
    ThreadLocalBase::operator=((uword)value);
    return *this;
  }

  friend auto operator<<(std::ostream& stream, const LazyThreadLocal<T>& rhs) -> std::ostream& {
    return stream << *rhs.Get();
  }
};
}  // namespace scm

#endif  // SCM_THREAD_LOCAL_H
