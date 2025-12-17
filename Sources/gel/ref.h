#ifndef GEL_REF_H
#define GEL_REF_H

#include "gel/common.h"
#include "gel/type/object.h"

namespace gel {
class RefBase {
  // TODO @s0cks: implement Reference counting for ptr_
  DEFINE_DEFAULT_COPYABLE_TYPE(RefBase);

 private:
  Object* ptr_;

 protected:
  explicit constexpr RefBase(Object* value = UNALLOCATED) :
    ptr_(value) {}

  inline constexpr auto ptr() const -> Object* {
    return ptr_;
  }

 public:
  ~RefBase() = default;

  inline constexpr auto IsAllocated() const -> bool {
    return ptr() != UNALLOCATED;
  }
};

template <class T>
class Ref : public RefBase {
  DEFINE_DEFAULT_COPYABLE_TYPE(Ref<T>);

 public:
  explicit constexpr Ref(const T* value) :
    RefBase((Object*)value) {}
  ~Ref() = default;

  inline constexpr auto value() const -> T* {
    return (T*)ptr();
  }

  inline constexpr auto operator->() -> T* {
    return value();
  }

  inline constexpr operator bool() const {
    return IsAllocated();
  }

  inline constexpr auto operator==(const Ref<T>& rhs) const -> bool {
    if (IsAllocated()) {
      if (!rhs.IsAllocated())
        return false;
      ASSERT(rhs.IsAllocated());
      return ptr()->Equals(rhs->ptr());
    }
    ASSERT(!IsAllocated());
    if (rhs.IsAllocated())
      return false;
    ASSERT(!rhs.IsAllocated());
    return true;
  }

  inline constexpr auto operator!=(const Ref<T>& rhs) const -> bool {
    return !operator==(rhs);
  }
};
}  // namespace gel

#endif  // GEL_REF_H
