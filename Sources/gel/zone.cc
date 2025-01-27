#include "gel/zone.h"

#include <units.h>

#include "gel/common.h"
#include "gel/free_list.h"
#include "gel/platform.h"
#include "gel/pointer.h"

namespace gel {
DEFINE_uword(new_zone_size, 4 * 1024 * 1024, "The size of the new zone.");
DEFINE_uword(old_zone_size, 4 * 1024 * 1024, "The initial size of the old zone (tenured & large object space).");

using namespace units::data;

static inline auto CalcSemispaceSize(const uword size) -> uword {
  ASSERT(IsPow2(size));
  return size / 2;
}

NewZone::NewZone(const uword size) :
  Zone(size, MemoryRegion::kReadWrite),
  fromspace_(GetStartingAddress()),
  tospace_(GetStartingAddress() + CalcSemispaceSize(size)),
  semi_size_(CalcSemispaceSize(size)) {}

auto NewZone::TryAllocate(const uword size) -> uword {
  ASSERT(size > 0);
  const auto total_size = (sizeof(Pointer) + size);
  if ((GetCurrentAddress() + total_size) >= (fromspace() + semisize())) {
    LOG(WARNING) << "cannot allocate " << byte_t(static_cast<double>(total_size)) << " in: " << (*this);
    // TODO: minor collection
  }
  if ((GetCurrentAddress() + total_size) >= (fromspace() + semisize())) {
    LOG(FATAL) << "cannot allocate " << byte_t(static_cast<double>(total_size)) << " in: " << (*this);
  }
  const auto new_address = GetCurrentAddress();
  current_ += total_size;
  memset((void*)new_address, 0, total_size);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  const auto new_ptr = Pointer::New(new_address, size);
  ASSERT(new_ptr);
#ifdef GEL_DEBUG
  memset(new_ptr->GetObjectAddressPointer(), 0, new_ptr->GetObjectSize());
#endif  // GEL_DEBUG
  return new_ptr->GetObjectAddress();
}

auto NewZone::VisitAllPointers(const std::function<bool(Pointer*)>& vis, const Tag filter) const -> bool {
  ASSERT(vis);
  Iterator iter(*this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    if ((next->GetTag() & filter) != filter)
      continue;
    if (!vis(next))
      return false;
  }
  return true;
}

auto NewZone::VisitAllPointers(PointerVisitor* vis) const -> bool {
  ASSERT(vis);
  Iterator iter(*this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    if (!vis->Visit(next))
      return false;
  }
  return true;
}

auto NewZone::VisitAllMarkedPointers(PointerVisitor* vis) const -> bool {
  ASSERT(vis);
  Iterator iter(*this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    const auto& tag = next->GetTag();
    if (tag.IsMarked()) {
      if (!vis->Visit(next))
        return false;
    }
  }
  return true;
}

OldZone::OldZone(const uword size) :
  Zone(size, MemoryRegion::kReadWrite),
  free_list_(GetStartingAddress(), size) {}

auto OldZone::TryAllocatePointer(const uword size) -> Pointer* {
  ASSERT(size > 0);
  const auto new_address = free_list_.TryAllocate(size);
  if (new_address == UNALLOCATED)
    return nullptr;
  const auto new_ptr = Pointer::Old(new_address, size);
  ASSERT(new_ptr);
#ifdef GEL_DEBUG
  memset(new_ptr->GetObjectAddressPointer(), 0, new_ptr->GetObjectSize());
#endif  // GEL_DEBUG
  return new_ptr;
}

auto OldZone::TryAllocate(const uword size) -> uword {
  const auto new_ptr = TryAllocatePointer(size);
  return new_ptr ? new_ptr->GetObjectAddress() : UNALLOCATED;
}

#ifdef GEL_DEBUG

void PrintNewZone(const NewZone& zone) {
  DLOG(INFO) << "New Zone:";
  DLOG(INFO) << "  Starting Address: " << zone.GetStartingAddressPointer();
  DLOG(INFO) << "  Total Size: " << PrettyPrintBytes(zone.GetSize());
  DLOG(INFO) << "  Semispace Size: " << PrettyPrintBytes(zone.semisize());
  DLOG(INFO) << "  Fromspace: " << zone.GetFromspacePointer();
  DLOG(INFO) << "  Tospace: " << zone.GetTospacePointer();
  DLOG(INFO) << "  Total Allocated: " << PrettyPrintBytes(zone.GetNumberOfBytesAllocated()) << " / "
             << zone.GetAllocationPercent();
}

void PrintOldZone(const OldZone& zone) {
  DLOG(INFO) << "Old Zone:";
  DLOG(INFO) << "  Starting Address: " << zone.GetStartingAddressPointer();
  DLOG(INFO) << "  Total Size: " << PrettyPrintBytes(zone.GetSize());
  DLOG(INFO) << "  Total Allocated: " << PrettyPrintBytes(zone.GetNumberOfBytesAllocated()) << " / "
             << zone.GetAllocationPercent();
  PrintFreeList(zone.GetFreeList());
}

#endif  // GEL_DEBUG
}  // namespace gel