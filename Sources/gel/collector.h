#ifndef GEL_COLLECTOR_H
#define GEL_COLLECTOR_H

#include "gel/common.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/zone.h"

namespace gel {
auto VisitRoots(PointerPointerVisitor* vis) -> bool;
auto VisitRoots(const std::function<bool(Pointer**)>& vis) -> bool;

class Collector : public PointerPointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(Collector);

 private:
  Heap& heap_;
  uword curr_address_ = UNALLOCATED;
  uword next_address_ = UNALLOCATED;

  inline auto heap() const -> Heap& {
    return heap_;
  }

  inline auto current_address() const -> uword {
    return curr_address_;
  }

  inline auto current_ptr() const -> Pointer* {
    return Pointer::At(current_address());
  }

  inline auto next_address() const -> uword {
    return next_address_;
  }

 protected:
  void ProcessRoots();
  auto ProcessFromspace() -> bool;
  auto Process(Pointer** ptr) -> bool;
  auto NotifyRoot(Pointer** ptr) -> bool;
  auto CopyPointer(Pointer* ptr) -> Pointer*;
  void NotifyRoots();
  auto Visit(Pointer** ptr) -> bool override;

 public:
  explicit Collector(Heap& heap);
  ~Collector() override = default;
  void Collect();
};

void MinorCollection();
void MajorCollection();
}  // namespace gel

#endif  // GEL_COLLECTOR_H
