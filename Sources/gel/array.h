#ifndef GEL_ARRAY_H
#define GEL_ARRAY_H

#include <string>

#include "gel/common.h"
#include "gel/object.h"
#include "gel/platform.h"
#include "gel/pointer.h"

namespace gel {
class Pointer;
class ArrayBase : public Object {
  template <typename T>
  friend class Array;

  friend class Object;
  friend class ArrayPointerIterator;

  static constexpr const auto kDefaultInitCapacity = 10;
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

    auto current_index() const -> uword {
      return index_;
    }

    auto HasNext() const -> bool {
      return index_ < array()->GetLength();
    }

    auto Next() -> Pointer** {
      const auto next = array()->GetPtrAddrAt(current_index());
      ASSERT(next);
      index_ += 1;
      return next;
    }
  };

 private:
  uword capacity_ = 0;
  uword length_ = 0;
  uword data_ = UNALLOCATED;

  auto data_address() const -> uword {
    ASSERT(data_ != UNALLOCATED);
    return data_;
  }

  inline auto data() const -> void* {
    return (void*)data_address();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto GetPtrAddrAt(const uword idx) const -> Pointer** {
    return (Pointer**)(data_address() + (idx * sizeof(uword)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

 protected:
  explicit ArrayBase(const word init_cap);

  void SetCapacity(const uword cap) {
    capacity_ = cap;
  }

  void Resize(const word new_length);
  auto UpdateForwardingPointers() -> bool;

 public:
  ~ArrayBase() override;

  auto IsArray() const -> bool override {
    return true;
  }

  auto GetType() const -> Class* override {
    ASSERT(kClass);
    return kClass;
  }

  auto GetCapacity() const -> uword {
    return capacity_;
  }

  auto GetLength() const -> uword {
    return length_;
  }

  auto HashCode() const -> uword override;
  auto Equals(Object* rhs) const -> bool override;
  auto ToString() const -> std::string override;

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;
  auto VisitValues(const std::function<bool(Object*)>& vis) -> bool;

 public:
  static void Init();
  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }

  static auto operator new(const size_t sz) -> void*;
  static void operator delete(void* ptr) {
    // do nothing
  }
};

template <typename T>
class Array : public ArrayBase {
  DEFINE_NON_COPYABLE_TYPE(Array<T>);

 public:
  Array(const word init_cap = kDefaultInitCapacity) :
    ArrayBase(init_cap) {}
  ~Array() override = default;

  void Clear() {
    length_ = 0;
  }

  auto IsEmpty() const -> bool {
    return GetLength() == 0;
  }

  void Push(T value) {
    ASSERT(value);
    Resize(static_cast<word>(GetLength() + 1));
    Set(GetLength() - 1, value);
  }

  auto Get(const uword idx) const -> T {
    ASSERT(idx >= 0 && idx <= GetCapacity());
    const auto ptr = GetPtrAddrAt(idx);
    return (T)((*ptr) ? (*ptr)->GetObjectPointer() : nullptr);
  }

  void Set(const uword idx, T value) {
    ASSERT(value);
    ASSERT(idx >= 0 && idx <= GetCapacity());
    *(GetPtrAddrAt(idx)) = value->raw_ptr();
  }

  void AddAll(Array<T>* rhs) {
    for (auto idx = 0; idx < rhs->GetLength(); idx++) {
      const auto value = rhs->Get(idx);
      ASSERT(value);
      Push(value);
    }
  }

  auto FindIf(const std::function<bool(T)>& filter) const -> T {
    for (auto idx = 0; idx < GetLength(); idx++) {
      const auto val = Get(idx);
      ASSERT(val);
      if (filter(val))
        return val;
    }
    return (T)UNALLOCATED;
  }

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
  static inline auto New(const word init_cap = kDefaultInitCapacity) -> Array<T>* {
    ASSERT(init_cap >= 0);
    return new Array<T>(init_cap);
  }
};
}  // namespace gel

#endif  // GEL_ARRAY_H
