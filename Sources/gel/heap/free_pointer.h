#ifndef GEL_FREE_POINTER_H
#define GEL_FREE_POINTER_H

#include <ostream>
#include <string>

#include "gel/common.h"
#include "gel/heap/region.h"
#include "gel/heap/tag.h"

namespace gel {
class FreePointer;
DECLARE_VISITOR(FreePointer);

class FreePointer {
  friend class FreeList;
  friend class FreeListTest;
  DEFINE_NON_COPYABLE_TYPE(FreePointer);

 private:
  Tag tag_;
  uword next_ = UNALLOCATED;

 protected:
  FreePointer(const Tag& tag) :
    tag_(tag) {}

  void SetNext(FreePointer* ptr) {
    next_ = ptr ? ptr->GetStartingAddress() : UNALLOCATED;
  }

 public:
  ~FreePointer() = default;

  auto tag() -> Tag& {
    return tag_;
  }

  auto tag() const -> const Tag& {
    return tag_;
  }

  auto GetStartingAddress() const -> uword {
    return (uword)this;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetStartingAddressPointer() const -> void* {
    return (void*)GetStartingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetObjectAddress() const -> uword {
    return GetStartingAddress() + sizeof(FreePointer);
  }

  auto GetObjectAddressPointer() const -> void* {
    return (void*)GetObjectAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetEndingAddress() const -> uword {
    return GetStartingAddress() + GetTotalSize();
  }

  auto GetEndingAddressPointer() const -> void* {
    return (void*)GetEndingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  //----------------------------------------------------------
  // TODO: these functions are confusing for judging size and i think it leaks data as:
  //  GetTotalSize() := (sizeof(FreePointer) + GetPointerSize());
  // where GetPointerSize() is already the sie of the available memory region
  inline auto GetPointerSize() const -> uword {
    return tag().GetSize();
  }

  inline auto GetTotalSize() const -> uword {
    return sizeof(FreePointer) + GetPointerSize();
  }
  //----------------------------------------------------------

  auto GetNext() const -> FreePointer* {
    return (FreePointer*)next_;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto HasNext() const -> bool {
    return GetNext() != nullptr;
  }

  inline auto region() const -> Region {
    return {GetStartingAddress(), GetPointerSize()};
  }

  auto Equals(const Region& region) const -> bool;
  auto Equals(FreePointer* rhs) const -> bool;
  auto ToString() const -> std::string;
  friend auto operator<<(std::ostream& stream, const FreePointer& rhs) -> std::ostream&;

 private:
  static inline auto New(const uword address, const Tag& tag) -> FreePointer* {
    ASSERT(address > UNALLOCATED);
    return new ((void*)address) FreePointer(tag);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  static inline auto New(const uword address, const uword size) -> FreePointer* {
    ASSERT(address > UNALLOCATED);
    return New(address, Tag::OldFree(size));
  }

  static inline auto New(void* ptr, const uword size) -> FreePointer* {
    ASSERT(ptr);
    return New((uword)ptr, size);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

 public:
  static auto At(const uword address) -> FreePointer* {
    return (FreePointer*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }
};
}  // namespace gel

#endif  // GEL_FREE_POINTER_H
