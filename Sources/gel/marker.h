#ifndef GEL_MARKER_H
#define GEL_MARKER_H

#include <deque>

#include "common.h"
#include "pointer.h"

namespace gel {
class Marker : public PointerVisitor {
  using WorkQueue = std::deque<uword>;
  DEFINE_NON_COPYABLE_TYPE(Marker);

 private:
  WorkQueue work_{};
  uword num_visited_{};
  uword num_marked_{};

  inline auto work() -> WorkQueue& {
    return work_;
  }

  inline auto HasWork() const -> bool {
    return !work_.empty();
  }

  inline auto NextAddress() -> uword {
    ASSERT(HasWork());
    const auto next = work_.front();
    work_.pop_front();
    return next;
  }

 protected:
  void Mark(Pointer* ptr);
  auto Visit(Pointer* ptr) -> bool override;

 public:
  Marker() = default;
  ~Marker() override = default;

  auto GetNumberOfObjectsMarked() const -> uword {
    return num_marked_;
  }

  auto GetNumberOfObjectsVisited() const -> uword {
    return num_visited_;
  }

  auto GetMarkedPercentage() const -> Percent {
    return Percent(GetNumberOfObjectsMarked(), GetNumberOfObjectsVisited());
  }

  auto MarkRoots() -> bool;
};
}  // namespace gel

#endif  // GEL_MARKER_H
