#ifndef GEL_HEAP_H
#define GEL_HEAP_H

#include <units.h>

#include "gel/common.h"
#include "gel/section.h"
#include "gel/zone.h"

namespace gel {
static constexpr const auto kLargeObjectSize = 4 * 1024;
class Heap {
  friend class Collector;
  DEFINE_NON_COPYABLE_TYPE(Heap);

 private:
  NewZone new_zone_;
  OldZone old_zone_;

  Heap();

  void Clear();

  inline auto new_zone() -> NewZone& {
    return new_zone_;
  }

  inline auto old_zone() -> OldZone& {
    return old_zone_;
  }

 public:
  ~Heap();

  auto TryAllocate(const uword size) -> uword;

  auto GetNewZone() const -> const NewZone& {
    return new_zone_;
  }

  auto GetOldZone() const -> const OldZone& {
    return old_zone_;
  }

  auto GetTotalSize() const -> uword {
    return new_zone_.GetSize() + old_zone_.GetSize();
  }

  friend auto operator<<(std::ostream& stream, const Heap& rhs) -> std::ostream& {
    stream << "Heap(";
    stream << "new_zone=" << rhs.new_zone_ << ", ";
    stream << "old_zone=" << rhs.old_zone_;
    stream << ")";
    return stream;
  }

 public:
  static auto GetHeap() -> Heap*;
  static void Init();
};

#ifdef GEL_DEBUG

void PrintHeap(Heap& heap);

#endif  // GEL_DEBUG

}  // namespace gel

#endif  // GEL_HEAP_H
