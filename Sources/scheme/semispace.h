#ifndef SCM_SEMISPACE_H
#define SCM_SEMISPACE_H

#include <units.h>

#include "scheme/common.h"
#include "scheme/pointer.h"
#include "scheme/section.h"

namespace scm {
class Semispace : public AllocationSection {
  DEFINE_DEFAULT_COPYABLE_TYPE(Semispace);

 public:
  class Iterator : public PointerIterator {
    DEFINE_NON_COPYABLE_TYPE(Iterator);

   private:
    const Semispace& semispace_;
    uword current_;

   public:
    explicit Iterator(const Semispace& semispace) :
      semispace_(semispace),
      current_(semispace.GetStartingAddress()) {}
    ~Iterator() = default;

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
    AllocationSection(start, size) {}
  virtual ~Semispace() = default;

  auto TryAllocate(const uword size) -> uword override;
  auto VisitAllPointers(PointerVisitor* vis) -> bool;
  auto VisitAllMarkedPointers(PointerVisitor* vis) -> bool;

  auto operator==(const Semispace& rhs) const -> bool {
    return ((const AllocationSection&)*this) == ((const AllocationSection&)rhs);
  }

  auto operator!=(const Semispace& rhs) const -> bool {
    return ((const AllocationSection&)*this) != ((const AllocationSection&)rhs);
  }

  friend auto operator<<(std::ostream& stream, const Semispace& rhs) -> std::ostream& {
    using namespace units::data;
    stream << "Semispace(";
    stream << "start=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "size=" << rhs.GetSize() << ", ";
    stream << "num_allocated=" << byte_t(rhs.GetNumberOfBytesAllocated());
    stream << " (" << rhs.GetAllocationPercent() << "), ";
    stream << "num_remaining=" << byte_t(rhs.GetNumberOfBytesRemaining());
    stream << " (" << rhs.GetRemainingPercent() << ")";
    stream << ")";
    return stream;
  }
};
}  // namespace scm

#endif  // SCM_SEMISPACE_H
