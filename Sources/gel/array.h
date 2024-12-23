#ifndef GEL_ARRAY_H
#define GEL_ARRAY_H

#include <string>

#include "gel/common.h"
#include "gel/object.h"

namespace gel {
class Pointer;
class ArrayBase : public Object {
  template <typename T>
  friend class Array;

  friend class Object;
  friend class ArrayPointerIterator;
  DEFINE_NON_COPYABLE_TYPE(ArrayBase);

 private:
  static Class* kClass;
  static void InitClass();
  static auto CreateClass() -> Class*;

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
  uword capacity_;

  inline auto GetStartingAddress() const -> uword {
    return (uword)this;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

 protected:
  explicit ArrayBase(const uword init_cap) :
    capacity_(init_cap) {
    memset(data(), 0, sizeof(uword) * init_cap);
  }

  auto data() const -> uword* {
    return (uword*)(GetStartingAddress() + sizeof(ArrayBase));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  void SetCapacity(const uword cap) {
    capacity_ = cap;
  }

  auto VisitPointers(const std::function<bool(Pointer**)>& vis) -> bool;

  auto GetPointerAt(const uword idx) const -> Pointer** {
    ASSERT(idx >= 0 && idx <= GetCapacity());
    return (Pointer**)&data()[idx];  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

 public:
  ~ArrayBase() override = default;

  auto GetCapacity() const -> uword {
    return capacity_;
  }

  auto IsArray() const -> bool override {
    return true;
  }

  auto GetType() const -> Class* override {
    return GetClass();
  }

  auto Get(const uword idx) const -> Object* {
    ASSERT(idx >= 0 && idx <= GetCapacity());
    const auto ptr = GetPointerAt(idx);
    return (*ptr) ? (*ptr)->GetObjectPointer() : nullptr;
  }

  void Set(const uword idx, Object* value) {
    ASSERT(value);
    ASSERT(idx >= 0 && idx <= GetCapacity());
    (*GetPointerAt(idx)) = value->raw_ptr();
  }

  auto operator[](const uword idx) const -> uword& {
    ASSERT(idx >= 0 && idx <= GetCapacity());
    return data()[idx];
  }

  auto HashCode() const -> uword override;
  auto Equals(Object* rhs) const -> bool override;
  auto ToString() const -> std::string override;

 public:
  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }

  static auto operator new(const size_t sz, const uword capacity) -> void*;
  static void operator delete(void* ptr) {
    // do nothing
  }
};

template <typename T>
class Array : public ArrayBase {
  DEFINE_NON_COPYABLE_TYPE(Array<T>);

 protected:
  Array() = default;

 public:
  ~Array() override;

  auto operator[](const uword idx) const -> T& {
    return (T&)data()[idx];
  }

  auto ToString() const -> std::string override;

  friend auto operator<<(std::ostream& stream, const Array<T>& rhs) -> std::ostream& {
    stream << "Array(";
    stream << "capacity=" << rhs.GetCapacity() << ", ";
    stream << "data=";
    stream << "[";
    for (auto idx = 0; idx < rhs.GetCapacity(); idx++) {
      const auto& value = rhs[idx];
      PrintValue(stream, value);
      if (idx < (rhs.GetCapacity() - 1))
        stream << ", ";
    }
    stream << "]";
    stream << ")";
    return stream;
  }

 public:
  static auto New(const uword init_cap) -> Array<T>* {
    return ((Array<T>*)new (init_cap) ArrayBase(init_cap));
  }
};
}  // namespace gel

#endif  // GEL_ARRAY_H
