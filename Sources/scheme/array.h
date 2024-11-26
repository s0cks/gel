#ifndef SCM_ARRAY_H
#define SCM_ARRAY_H

#include "scheme/common.h"

namespace scm {
class Pointer;
class ArrayBase {
  friend class ArrayPointerIterator;
  DEFINE_NON_COPYABLE_TYPE(ArrayBase);

 protected:
  class ArrayPointerIterator {
    DEFINE_NON_COPYABLE_TYPE(ArrayPointerIterator);

   private:
    const ArrayBase* array_;
    uword index_ = 0;

   public:
    explicit ArrayPointerIterator(const ArrayBase* array) :
      array_(array) {
      ASSERT(array_);
    }
    ~ArrayPointerIterator() = default;

    auto array() const -> const ArrayBase* {
      return array_;
    }

    auto HasNext() const -> bool {
      return index_ < array()->GetCapacity();
    }

    auto Next() -> Pointer** {
      const auto next = (Pointer**)&array()->data()[index_];  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
      ASSERT(next);
      index_ += 1;
      return next;
    }
  };

 private:
  uword length_ = 0;
  uword capacity_ = 0;
  uword* data_ = UNALLOCATED;

 protected:
  ArrayBase() = default;

  auto data() const -> uword* {
    return data_;
  }

  void SetLength(const uword len) {
    length_ = len;
  }

  void SetCapacity(const uword cap) {
    capacity_ = cap;
  }

  void Resize(const uword new_cap);
  auto VisitPointers(const std::function<bool(Pointer**)>& vis) -> bool;

 public:
  virtual ~ArrayBase() = default;

  auto GetLength() const -> uword {
    return length_;
  }

  auto GetCapacity() const -> uword {
    return capacity_;
  }

  inline void Clear() {
    return SetLength(0);
  }

  inline auto IsEmpty() const -> bool {
    return GetLength() == 0;
  }
};

template <typename T>
class Array : public ArrayBase {
  DEFINE_NON_COPYABLE_TYPE(Array<T>);

 protected:
  explicit Array(const uword init_cap) :
    ArrayBase() {
    if (init_cap > 0)
      Resize(init_cap);
  }

 public:
  ~Array();

  auto Last() const -> T& {
    return operator[](GetLength() - 1);
  }

  void Push(const T& rhs) {
    Resize(GetLength() + 1);
    Last() = rhs;
  }

  auto Pop() -> T& {
    const auto last = Last();
    SetLength(GetLength() - 1);
    return last;
  }

  auto operator[](const uword idx) const -> T& {
    return (T&)data()[idx];
  }

 public:
  static auto New(const uword init_cap) -> Array*;
};
}  // namespace scm

#endif  // SCM_ARRAY_H
