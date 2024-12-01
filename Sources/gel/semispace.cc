#include "gel/semispace.h"

namespace gel {
auto Semispace::TryAllocate(const uword size) -> uword {
  ASSERT(size > 0 && IsPow2(size));
  if ((current_ + size) >= GetEndingAddress())
    return UNALLOCATED;
  const auto next = current_;
  current_ += size;
  return next;
}

auto Semispace::VisitAllPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  Iterator iter(*this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next && next->GetObjectSize() > 0);
    if (!vis->Visit(next))
      return false;
  }
  return true;
}

auto Semispace::VisitAllMarkedPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  Iterator iter(*this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next && next->GetObjectSize() > 0);
    const auto& tag = next->GetTag();
    if (!tag.IsMarked())
      continue;
    if (!vis->Visit(next))
      return false;
  }
  return true;
}
}  // namespace gel