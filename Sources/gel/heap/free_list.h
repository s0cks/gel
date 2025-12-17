#ifndef GEL_FREE_LIST_H
#define GEL_FREE_LIST_H

#include <ostream>
#include <string>

#include "gel/common.h"
#include "gel/heap/free_pointer.h"
#include "gel/memory_region.h"
#include "gel/heap/pointer.h"
#include "gel/heap/region.h"
#include "gel/heap/tag.h"

namespace gel {
class FreeList : public Region {
  friend class OldZone;
  friend class FreeListTest;
  DEFINE_DEFAULT_COPYABLE_TYPE(FreeList);

 public:
  class Iterator {
    DEFINE_NON_COPYABLE_TYPE(Iterator);

   private:
    FreePointer* current_;

   public:
    explicit Iterator(FreePointer* head) :
      current_(head) {}
    explicit Iterator(const FreeList* free_list) :
      Iterator(free_list->head()) {}
    ~Iterator() = default;

    auto HasNext() const -> bool {
      return current_ != nullptr;
    }

    auto Next() -> FreePointer* {
      const auto next = current_;
      current_ = next->GetNext();
      return next;
    }
  };

 private:
  FreePointer* head_;

  inline void SetHead(FreePointer* rhs) {
    ASSERT(rhs);
    head_ = rhs;
  }

 protected:
  FreeList() :
    Region(),
    head_(nullptr) {}
  FreeList(const uword start_address, const uword size) :
    Region(start_address, size),
    head_(FreePointer::New(start_address, Tag::OldFree(size))) {
    ASSERT(head_);
  }
  FreeList(const MemoryRegion& region) :
    Region(region),
    head_(FreePointer::New(region.GetStartingAddress(), Tag::OldFree(region.GetSize()))) {
    ASSERT(head_);
  }

  inline auto GetHead() const -> FreePointer* {
    ASSERT(head_);
    return head_;
  }

  void Remove(FreePointer* free_ptr);
  auto FindBestFit(const uword size) -> FreePointer*;
  auto Split(FreePointer* free_ptr, const uword size) -> FreePointer*;

 public:
  ~FreeList() override = default;

  void Clear() override {
    Region::Clear();
    head_ = FreePointer::New(GetStartingAddress(), Tag::OldFree(GetSize()));
    ASSERT(head_);
  }

  auto head() const -> FreePointer* {
    return head_;
  }

  inline auto HasHead() const -> bool {
    return head() != nullptr;
  }

  auto IsEmpty() const -> bool {
    return head()->GetStartingAddress() == GetStartingAddress() && head()->GetPointerSize() == GetSize() &&
           !head()->HasNext();
  }

  void Remove(const Region& region);
  auto Insert(FreePointer* free_ptr) -> bool;
  auto TryAllocate(const uword size) -> uword;
  auto VisitFreePointers(FreePointerVisitor* vis) const -> bool;
  auto ToString() const -> std::string;

  friend auto operator<<(std::ostream& stream, const FreeList& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }
};

#ifdef GEL_DEBUG
void PrintFreeList(const FreeList& free_list);
#endif  // GEL_DEBUG
}  // namespace gel

#endif  // GEL_FREE_LIST_H
