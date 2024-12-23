#ifndef GEL_SEMISPACE_H
#define GEL_SEMISPACE_H

#include <units.h>

#include "gel/common.h"
#include "gel/pointer.h"
#include "gel/section.h"

namespace gel {
class Semispace : public AllocationRegion {
  DEFINE_DEFAULT_COPYABLE_TYPE(Semispace);

 public:
  class Iterator : public PointerIterator {
    DEFINE_NON_COPYABLE_TYPE(Iterator);

   private:
    const Semispace& semispace_;
    uword current_;

   public:
    explicit Iterator(const Semispace& semispace) :
      PointerIterator(),
      semispace_(semispace),
      current_(semispace.GetStartingAddress()) {}
    ~Iterator() override = default;

    auto HasNext() const -> bool override {
      return current_ < semispace_.GetCurrentAddress();
    }

    auto Next() -> Pointer* override {
      const auto next = Pointer::At(current_);
      current_ += next->GetTotalSize();
      return next;
    }
  };

 public:
  Semispace() = default;
  Semispace(const uword start, const uword size) :
    AllocationRegion(start, size) {}
  ~Semispace() override = default;

  auto TryAllocate(const uword size) -> uword override;
  auto VisitAllPointers(PointerVisitor* vis) -> bool;
  auto VisitAllMarkedPointers(PointerVisitor* vis) -> bool;

  auto operator==(const Semispace& rhs) const -> bool {
    return ((const AllocationRegion&)*this) == ((const AllocationRegion&)rhs);
  }

  auto operator!=(const Semispace& rhs) const -> bool {
    return ((const AllocationRegion&)*this) != ((const AllocationRegion&)rhs);
  }

  friend auto operator<<(std::ostream& stream, const Semispace& rhs) -> std::ostream& {
    using namespace units::data;
    stream << "Semispace(";
    stream << "start=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "size=" << rhs.GetSize() << ", ";
    stream << "num_allocated=" << byte_t(static_cast<double>(rhs.GetNumberOfBytesAllocated()));
    stream << " (" << rhs.GetAllocationPercent() << "), ";
    stream << "num_remaining=" << byte_t(static_cast<double>(rhs.GetNumberOfBytesRemaining()));
    stream << " (" << rhs.GetRemainingPercent() << ")";
    stream << ")";
    return stream;
  }
};
}  // namespace gel

#endif  // GEL_SEMISPACE_H
