#ifndef GEL_COLLECTOR_H
#define GEL_COLLECTOR_H

#include <functional>

#include "gel/common.h"
#include "gel/heap/pointer.h"
#include "gel/heap/zone.h"

namespace gel {
class Collector : public PointerPointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(Collector);

 public:
  static auto VisitRoots(PointerPointerVisitor* vis) -> bool;
  static auto VisitRoots(const std::function<bool(Pointer**)>& vis) -> bool;

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
  auto ProcessRoots() -> bool;
  auto ProcessFromspace() -> bool;

  auto Promote(Pointer* ptr) -> uword;
  auto Scavenge(Pointer* ptr) -> uword;

  auto ProcessPointer(Pointer* ptr) -> uword;
  auto Process(Pointer** ptr) -> bool;
  auto Visit(Pointer** ptr) -> bool override;

 public:
  explicit Collector(Heap& heap);
  ~Collector() override = default;
  void Collect();
};

void MinorCollection();
void MajorCollection();

#ifdef GEL_DEBUG
void PrintRoots();
#endif  // GEL_DEBUG
}  // namespace gel

#endif  // GEL_COLLECTOR_H
