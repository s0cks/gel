#ifndef SCM_HEAP_H
#define SCM_HEAP_H

#include <units.h>

#include "scheme/common.h"
#include "scheme/section.h"
#include "scheme/zone.h"

namespace scm {
static constexpr const auto kLargeObjectSize = 4 * 1024;
class Heap {
  DEFINE_NON_COPYABLE_TYPE(Heap);

 private:
  NewZone new_zone_;
  OldZone old_zone_;

  Heap();

  void Clear();

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

#ifdef SCM_DEBUG

void PrintHeap(Heap& heap);

#endif  // SCM_DEBUG

}  // namespace scm

#endif  // SCM_HEAP_H
