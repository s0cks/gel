#ifndef GEL_ALLOCATOR_H
#define GEL_ALLOCATOR_H

#include <string>

#include "gel/common.h"
#include "gel/platform.h"
#include "gel/pointer.h"

namespace gel {
class Allocator {
  DEFINE_NON_INSTANTIABLE_TYPE(Allocator);

 public:
  auto malloc(const uword size) -> void*;
  auto realloc(const void* ptr, const uword size) -> void*;
  void free(const void* ptr);
};

class Pointer;
class PointerVisitor;
class PointerPointerVisitor;
class HeapObject {
  DEFINE_NON_COPYABLE_TYPE(HeapObject);

 protected:
  HeapObject() = default;

  virtual auto VisitPointers(PointerVisitor* vis) -> bool {
    ASSERT(vis);
    // do nothing
    return true;
  }

  virtual auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
    ASSERT(vis);
    // do nothing
    return true;
  }

  template <class T>
  static inline auto VisitPointerPointer(PointerPointerVisitor* vis, T** field) -> bool {
    ASSERT(vis);
    const auto old_value = (*field);
    if (!old_value)
      return true;
    Pointer* ptr = old_value->raw_ptr();
    if (!vis->Visit(&ptr))
      return false;
    if (!(*field)->raw_ptr()->Equals(ptr)) {
      const auto new_value = ptr->As<T>();
      ASSERT(new_value);
      (*field) = new_value;
    }
    return true;
  }

 public:  // TODO: reduce visibility
  auto raw_ptr() const -> Pointer*;

  auto GetStartingAddress() const -> uword {
    return (uword)this;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto GetStartingAddressPointer() const -> void* {
    return ((void*)GetStartingAddress());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

 public:
  virtual ~HeapObject() = default;
  virtual auto ToString() const -> std::string = 0;
};

#define DECLARE_HEAP_ALLOC_TYPE(Name)        \
  DEFINE_NON_COPYABLE_TYPE(Name);            \
                                             \
 public:                                     \
  auto operator new(const size_t sz)->void*; \
  void operator delete(void* ptr);

#ifdef GEL_DISABLE_HEAP

#define DEFINE_NEW_OPERATOR(Name)                     \
  auto Name::operator new(const size_t sz) -> void* { \
    return malloc(sz);                                \
  }

#else

// TODO: add assertions for Name::kClass && sz <= kClass->GetAllocationSize()
#define DEFINE_NEW_OPERATOR(Name)                                             \
  auto Name::operator new(const size_t sz) -> void* {                         \
    const auto alloc_size = Name::kClass ? kClass->GetAllocationSize() : sz;  \
    const auto heap = GetCurrentThreadHeap();                                 \
    ASSERT(heap);                                                             \
    const auto address = heap->TryAllocate(alloc_size > 0 ? alloc_size : sz); \
    ASSERT(address != UNALLOCATED);                                           \
    return reinterpret_cast<void*>(address);                                  \
  }

#endif  // GEL_DISABLE_HEAP

namespace sys {
class Allocator {
  DEFINE_NON_INSTANTIABLE_TYPE(Allocator);

 public:
  auto malloc(const uword size) -> void*;
  auto realloc(const void* ptr, const uword size) -> void*;
  void free(const void* ptr);
};
}  // namespace sys
}  // namespace gel

#endif  // GEL_ALLOCATOR_H
