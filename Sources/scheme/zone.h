#ifndef SCM_ZONE_H
#define SCM_ZONE_H

#include "scheme/common.h"
#include "scheme/flags.h"
#include "scheme/memory_region.h"
#include "scheme/pointer.h"
#include "scheme/section.h"
#include "scheme/semispace.h"

namespace scm {
class Zone : public AllocationSection {
  DEFINE_DEFAULT_COPYABLE_TYPE(Zone);

 protected:
  Zone() = default;
  explicit Zone(const MemoryRegion& region) :
    AllocationSection(region.GetStartingAddress(), region.GetSize()) {}
  explicit Zone(const uword size, const MemoryRegion::ProtectionMode mode = MemoryRegion::kReadOnly) :
    Zone(MemoryRegion(size, mode)) {}

  void Protect(const MemoryRegion::ProtectionMode mode) {
    MemoryRegion region(*this);
    region.Protect(mode);
  }

  inline void SetReadOnly() {
    return Protect(MemoryRegion::kReadOnly);
  }

  inline void SetWritable() {
    return Protect(MemoryRegion::kReadWrite);
  }

 public:
  ~Zone() override = default;

  friend auto operator<<(std::ostream& stream, const Zone& rhs) -> std::ostream& {
    stream << "Zone(";
    stream << "starting_address=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "size=" << rhs.GetSize();
    stream << ")";
    return stream;
  }
};

DECLARE_uword(new_zone_size);

static inline auto GetNewZoneSize() -> uword {
  return FLAGS_new_zone_size;
}

class NewZone : public Zone {
  friend class Heap;
  DEFINE_DEFAULT_COPYABLE_TYPE(NewZone);

 public:
  class Iterator : public PointerIterator {
    DEFINE_NON_COPYABLE_TYPE(Iterator);

   private:
    const NewZone& new_zone_;
    uword current_;

   public:
    explicit Iterator(const NewZone& new_zone) :
      PointerIterator(),
      new_zone_(new_zone),
      current_(new_zone.GetStartingAddress()) {}  // TODO: this causes issues when semispaces get flipped from initial positions
    ~Iterator() = default;

    auto HasNext() const -> bool override {
      return current_ < new_zone_.GetCurrentAddress();
    }

    auto Next() -> Pointer* override {
      const auto next = Pointer::At(current_);
      ASSERT(next);
      current_ += next->GetTotalSize();
      return next;
    }
  };

 private:
  uword fromspace_;
  uword tospace_;
  uword semi_size_;

  explicit NewZone(const uword size = GetNewZoneSize(), const MemoryRegion::ProtectionMode mode = MemoryRegion::kReadOnly);

 public:
  ~NewZone() override = default;

  auto fromspace() const -> uword {
    return fromspace_;
  }

  auto tospace() const -> uword {
    return tospace_;
  }

  auto semisize() const -> uword {
    return semi_size_;
  }

  auto VisitAllPointers(PointerVisitor* vis) const -> bool;
  auto VisitAllMarkedPointers(PointerVisitor* vis) const -> bool;
  auto TryAllocate(const uword size) -> uword override;

  auto GetAllocationPercent() const -> Percent override {
    return Percent(GetPercentageOf(GetNumberOfBytesAllocated(), semisize()));
  }

  friend auto operator<<(std::ostream& stream, const NewZone& rhs) -> std::ostream& {
    using namespace units::data;
    stream << "NewZone(";
    stream << "start=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "size=" << byte_t(rhs.GetSize()) << ", ";
    stream << "fromspace=" << ((void*)rhs.fromspace()) << ", ";
    stream << "to=" << ((void*)rhs.tospace()) << ", ";
    stream << "semi_size=" << byte_t(rhs.semisize()) << ", ";
    stream << "allocated=" << byte_t(rhs.GetNumberOfBytesAllocated());
    stream << " (" << rhs.GetAllocationPercent() << "), ";
    stream << "remaining=" << byte_t(rhs.GetNumberOfBytesRemaining());
    stream << " (" << rhs.GetRemainingPercent() << ")";
    stream << ")";
    return stream;
  }
};

class OldZone : public Zone {
  friend class Heap;
  DEFINE_DEFAULT_COPYABLE_TYPE(OldZone);

 public:
  OldZone() = default;
  explicit OldZone(const uword size, const MemoryRegion::ProtectionMode mode = MemoryRegion::kReadOnly);
  ~OldZone() override = default;

  auto TryAllocate(const uword size) -> uword override;

  friend auto operator<<(std::ostream& stream, const OldZone& rhs) -> std::ostream& {
    using namespace units::data;
    stream << "NewZone(";
    stream << "start=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "size=" << rhs.GetSize() << ", ";
    stream << "allocated=" << byte_t(rhs.GetNumberOfBytesAllocated());
    stream << " (" << rhs.GetAllocationPercent() << "), ";
    stream << "remaining=" << byte_t(rhs.GetNumberOfBytesRemaining());
    stream << " (" << rhs.GetRemainingPercent() << ")";
    stream << ")";
    return stream;
  }
};

#ifdef SCM_DEBUG

void PrintNewZone(const NewZone& zone);
void PrintOldZone(const OldZone& zone);

#endif  // SCM_DEBUG
}  // namespace scm

#endif  // SCM_ZONE_H
