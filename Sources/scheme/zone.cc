#include "scheme/zone.h"

#include <units.h>

#include "scheme/common.h"
#include "scheme/platform.h"
#include "scheme/pointer.h"

namespace scm {
DEFINE_uword(new_zone_size, 4 * 1024 * 1024, "The size of the new zone.");

using namespace units::data;

static inline auto CalcSemispaceSize(const uword size) -> uword {
  ASSERT(IsPow2(size));
  return size / 2;
}

NewZone::NewZone(const uword size, const MemoryRegion::ProtectionMode mode) :
  Zone(size, mode),
  fromspace_(GetStartingAddress()),
  tospace_(GetStartingAddress() + CalcSemispaceSize(size)),
  semi_size_(CalcSemispaceSize(size)) {
  Zone::SetWritable();
}

auto NewZone::TryAllocate(const uword size) -> uword {
  ASSERT(size > 0);
  const auto total_size = (sizeof(Pointer) + size);
  if ((GetCurrentAddress() + total_size) >= (fromspace() + semisize())) {
    LOG(WARNING) << "cannot allocate " << byte_t(total_size) << " in: " << (*this);
    // TODO: minor collection
  }
  if ((GetCurrentAddress() + total_size) >= (fromspace() + semisize())) {
    LOG(FATAL) << "cannot allocate " << byte_t(total_size) << " in: " << (*this);
  }
  const auto ptr = Pointer::New(GetCurrentAddress(), size);
  current_ += total_size;
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

auto OldZone::TryAllocate(const uword size) -> uword {
  ASSERT(size > 0);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return UNALLOCATED;
}

#ifdef SCM_DEBUG

using namespace units::data;

static inline auto PrettyPrintBytes(const uword num_bytes) -> std::string {
  std::stringstream ss;
  int scale = 0;
  uword remaining = num_bytes;
  while (remaining >= 1024) {
    remaining /= 1024;
    scale += 1;
  }
  switch (scale) {
    case 1:
      ss << kilobyte_t(remaining);
      break;
    case 2:
      ss << megabyte_t(remaining);
      break;
    case 3:
      ss << gigabyte_t(remaining);
      break;
    case 4:
      ss << terabyte_t(remaining);
      break;
    case 5:
      ss << petabyte_t(remaining);
      break;
    case 0:
    default:
      ss << byte_t(remaining);
      break;
  }
  return ss.str();
}

void PrintNewZone(const NewZone& zone) {
  DLOG(INFO) << "New Zone:";
  DLOG(INFO) << "  Total Size: " << PrettyPrintBytes(zone.GetSize());
  DLOG(INFO) << "  Semispace Size: " << PrettyPrintBytes(zone.semisize());
  DLOG(INFO) << "  Allocated: " << PrettyPrintBytes(zone.GetNumberOfBytesAllocated()) << " / " << zone.GetAllocationPercent();
}

void PrintOldZone(const OldZone& zone) {
  DLOG(INFO) << "Old Zone:";
  DLOG(INFO) << "  Total Size: " << PrettyPrintBytes(zone.GetSize());
  DLOG(INFO) << "  Allocated: " << PrettyPrintBytes(zone.GetNumberOfBytesAllocated()) << " / " << zone.GetAllocationPercent();
}

#endif  // SCM_DEBUG
}  // namespace scm