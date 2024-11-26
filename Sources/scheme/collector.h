#ifndef SCM_COLLECTOR_H
#define SCM_COLLECTOR_H

#include "scheme/common.h"
#include "scheme/platform.h"
#include "scheme/pointer.h"
#include "scheme/zone.h"

namespace scm {
class Collector {
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
  auto ProcessRoots() -> bool;
  auto ProcessFromspace() -> bool;
  auto ProcessRoot(Pointer** ptr) -> bool;
  auto CopyPointer(Pointer* ptr) -> Pointer*;

 public:
  explicit Collector(Heap& heap);
  virtual ~Collector() = default;
  void Collect();
};

void MinorCollection();
}  // namespace scm

#endif  // SCM_COLLECTOR_H
