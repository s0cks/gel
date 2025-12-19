#include "marker.h"

#include "module.h"
#include "object.h"
#include "platform.h"

namespace gel {
void Marker::Mark(Pointer* ptr) {
  ASSERT(ptr);
  num_marked_ += 1;
  ptr->SetMarked();
}

auto Marker::Visit(Pointer* ptr) -> bool {
  ASSERT(ptr);
  num_visited_ += 1;
  if (!ptr->IsMarked())
    work_.push_back(ptr->GetStartingAddress());
  return true;
}

auto Marker::MarkRoots() -> bool {
  if (!Class::VisitAllClassPointers(this)) {
    DLOG(ERROR) << "failed to visit classes.";
    return false;
  }

  // TODO: probably should only ever visit _kernel & imported modules from _kernel
  if (!Module::VisitAllModulePointers(this)) {
    DLOG(ERROR) << "failed to visit module pointers.";
    return false;
  }

  while (HasWork()) {
    const auto address = NextAddress();
    ASSERT(address != UNALLOCATED);
    const auto ptr = Pointer::At(address);
    ASSERT(ptr);
    Mark(ptr);
    LOG_IF(ERROR, !ptr->VisitPointers(this)) << "failed to visit: " << (*ptr);
    // TODO: probably should return if ptr->VisitPointers fails
  }
  return true;
}
}  // namespace gel
