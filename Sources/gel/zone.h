#ifndef GEL_ZONE_H
#define GEL_ZONE_H

#include "gel/common.h"
#include "gel/flags.h"
#include "gel/free_list.h"
#include "gel/memory_region.h"
#include "gel/pointer.h"
#include "gel/section.h"
#include "gel/semispace.h"

namespace gel {
class Zone : public AllocationRegion {
  DEFINE_DEFAULT_COPYABLE_TYPE(Zone);

 protected:
  Zone() = default;
  explicit Zone(const MemoryRegion& region) :
    AllocationRegion(region.GetStartingAddress(), region.GetSize()) {}
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

class Heap;
class Collector;
class NewZone : public Zone {
  friend class Heap;
  friend class Collector;
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
    ~Iterator() override = default;

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

  explicit NewZone(const uword size = GetNewZoneSize());

  inline void SwapSpaces() {
    std::swap(fromspace_, tospace_);
  }

 public:
  ~NewZone() override = default;

  auto fromspace() const -> uword {
    return fromspace_;
  }

  auto GetFromspacePointer() const -> void* {
    return (void*)fromspace();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto tospace() const -> uword {
    return tospace_;
  }

  auto GetTospacePointer() const -> void* {
    return (void*)tospace();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto semisize() const -> uword {
    return semi_size_;
  }

  auto VisitAllPointers(PointerVisitor* vis) const -> bool;
  auto VisitAllMarkedPointers(PointerVisitor* vis) const -> bool;
  auto TryAllocate(const uword size) -> uword override;

  auto GetNumberOfBytesAllocated() const -> uword override {
    return (GetCurrentAddress() - fromspace());
  }

  auto GetAllocationPercent() const -> Percent override {
    return Percent(GetNumberOfBytesAllocated(), semisize());
  }

  friend auto operator<<(std::ostream& stream, const NewZone& rhs) -> std::ostream& {
    using namespace units::data;
    stream << "NewZone(";
    stream << "start=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "size=" << byte_t(static_cast<double>(rhs.GetSize())) << ", ";
    stream << "fromspace=" << rhs.GetFromspacePointer() << ", ";
    stream << "to=" << rhs.GetTospacePointer() << ", ";
    stream << "semi_size=" << byte_t(static_cast<double>(rhs.semisize())) << ", ";
    stream << "allocated=" << byte_t(static_cast<double>(rhs.GetNumberOfBytesAllocated()));
    stream << " (" << rhs.GetAllocationPercent() << "), ";
    stream << "remaining=" << byte_t(static_cast<double>(rhs.GetNumberOfBytesRemaining()));
    stream << " (" << rhs.GetRemainingPercent() << ")";
    stream << ")";
    return stream;
  }
};

DECLARE_uword(old_zone_size);

static inline auto GetOldZoneSize() -> uword {
  return FLAGS_old_zone_size;
}

class OldZone : public Zone {
  friend class Heap;
  DEFINE_NON_COPYABLE_TYPE(OldZone);

 private:
  FreeList free_list_;

 public:
  explicit OldZone(const uword size = GetOldZoneSize());
  ~OldZone() override = default;

  auto free_list() const -> const FreeList& {
    return free_list_;
  }

  auto TryAllocate(const uword size) -> uword override;

  friend auto operator<<(std::ostream& stream, const OldZone& rhs) -> std::ostream& {
    using namespace units::data;
    stream << "NewZone(";
    stream << "start=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "size=" << rhs.GetSize() << ", ";
    stream << "allocated=" << byte_t(static_cast<double>(rhs.GetNumberOfBytesAllocated()));
    stream << " (" << rhs.GetAllocationPercent() << "), ";
    stream << "remaining=" << byte_t(static_cast<double>(rhs.GetNumberOfBytesRemaining()));
    stream << " (" << rhs.GetRemainingPercent() << ")";
    stream << ")";
    return stream;
  }
};

#ifdef GEL_DEBUG

void PrintNewZone(const NewZone& zone);
void PrintOldZone(const OldZone& zone);

#endif  // GEL_DEBUG
}  // namespace gel

#endif  // GEL_ZONE_H
