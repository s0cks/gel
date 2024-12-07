#include "gel/pointer.h"

#include "gel/object.h"

namespace gel {
auto Pointer::VisitPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  return GetObjectPointer()->VisitPointers(vis);
}
}  // namespace gel