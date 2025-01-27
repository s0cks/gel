#include "gel/free_list.h"

#include "gel/common.h"
#include "gel/free_pointer.h"
#include "gel/platform.h"
#include "gel/to_string_helper.h"

namespace gel {
auto FreeList::ToString() const -> std::string {
  ToStringHelper<FreeList> helper;
  helper.AddField("starting_address", GetStartingAddressPointer());
  helper.AddField("head", GetHead());
  helper.AddBytesField("total_size", GetSize());
  return helper;
}

auto FreeList::VisitFreePointers(FreePointerVisitor* vis) const -> bool {
  ASSERT(vis);
  Iterator iter(this);
  while (iter.HasNext()) {
    if (!vis->Visit(iter.Next()))
      return false;
  }
  return true;
}

void FreeList::Remove(const Region& region) {
  if (!Contains(region))
    return;
  if (HasHead() && GetHead()->Equals(region)) {
    auto head = GetHead();
    head_ = head->GetNext();
    return;
  }

  FreePointer* current = GetHead();
  FreePointer* previous = nullptr;
  while (current != nullptr && !current->Equals(region)) {
    previous = current;
    current = current->GetNext();
  }

  if (current != nullptr && previous)
    previous->SetNext(current->GetNext());
  return;
}

auto FreeList::Insert(FreePointer* free_ptr) -> bool {
  ASSERT(free_ptr);
  if (!Contains(free_ptr->region()))
    return false;
  if (!HasHead()) {
    head_ = free_ptr;
    return true;
  }
  auto node = head();
  FreePointer* previous = nullptr;
  while (node != nullptr && node->GetStartingAddress() <= free_ptr->GetStartingAddress()) {
    previous = node;
    node = node->GetNext();
  }
  if ((previous && previous->Equals(free_ptr)) || (node && node->Equals(free_ptr)))
    return false;
  if (!previous) {
    free_ptr->SetNext(head());
    head_ = free_ptr;
  } else {
    free_ptr->SetNext(node);
    previous->SetNext(free_ptr);
  }
  return true;
}

static inline auto CanSplit(FreePointer* free_ptr, const uword size) -> bool {
  return free_ptr && ((free_ptr->GetTotalSize() - size) >= kWordSize);
}

auto FreeList::FindBestFit(const uword total_size) -> FreePointer* {
  ASSERT(total_size > 0);
  FreePointer* current = GetHead();
  while (current && current->GetPointerSize() < total_size) {
    current = current->GetNext();
  }
  return current;
}

auto FreeList::Split(FreePointer* free_ptr, const uword size) -> FreePointer* {
  ASSERT(CanSplit(free_ptr, size));
  const auto new_size = free_ptr->GetPointerSize() - size;
  const auto new_start = free_ptr->GetStartingAddress() + size;
  return FreePointer::New(new_start, new_size);
}

auto FreeList::TryAllocate(const uword size) -> uword {
  ASSERT(size > 0);
  const auto total_size = (sizeof(Pointer) + size);
  const auto best_fit = FindBestFit(total_size);
  if (!best_fit) {
    LOG(ERROR) << "failed to find best fit in FreeList for " << PrettyPrintBytes(total_size);
    return UNALLOCATED;
  }
  ASSERT(best_fit);

  if (CanSplit(best_fit, total_size)) {
    const auto new_node = Split(best_fit, total_size);
    ASSERT(new_node);
    Insert(new_node);
  }

  Remove(best_fit->region());
  return best_fit->GetStartingAddress();
}

#ifdef GEL_DEBUG
void PrintFreeList(const FreeList& free_list) {
  LOG(INFO) << free_list << ": ";
  FreePointerVisitorWrapper vis([](FreePointer* free_ptr) {
    ASSERT(free_ptr);
    LOG(INFO) << "- " << free_ptr->ToString();
    return true;
  });
  LOG_IF(ERROR, !free_list.VisitFreePointers(&vis)) << "failed to visit: " << free_list;
}
#endif  // GEL_DEBUG
}  // namespace gel