#include "gel/zone.h"

#include <units.h>

#include "gel/common.h"
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
  memset((void*)new_address, 0, total_size);
  const auto ptr = Pointer::New(new_address, size);
  return ptr->GetObjectAddress();
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
  free_list_() {
  free_list_ = FreeList(GetStartingAddress(), size);
}

auto OldZone::TryAllocate(const uword size) -> uword {
  ASSERT(size > 0);
  return free_list_.TryAllocate(size);
}

#ifdef GEL_DEBUG

using namespace units::data;

static inline auto PrettyPrintBytes(const uword num_bytes) -> std::string {
  static constexpr const auto kScale = 1024;

  std::stringstream ss;
  int scale = 0;
  uword remaining = num_bytes;
  while (remaining >= kScale) {
    remaining /= kScale;
    scale += 1;
  }
  switch (scale) {
    case 1:
      ss << kilobyte_t(static_cast<double>(remaining));
      break;
    case 2:
      ss << megabyte_t(static_cast<double>(remaining));
      break;
    case 3:
      ss << gigabyte_t(static_cast<double>(remaining));
      break;
    case 4:
      ss << terabyte_t(static_cast<double>(remaining));
      break;
    case 5:  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
      ss << petabyte_t(static_cast<double>(remaining));
      break;
    case 0:
    default:
      ss << byte_t(static_cast<double>(remaining));
      break;
  }
  return ss.str();
}

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
  DLOG(INFO) << "  Free Pointers:";
  LOG_IF(FATAL, !zone.free_list().VisitFreePointers([](FreePointer* ptr) {
    DLOG(INFO) << "  - " << (*ptr);
    return true;
  })) << "failed to visit FreePointers in: "
      << zone;
}

#endif  // GEL_DEBUG
}  // namespace gel