#include "gel/pointer.h"

#include "gel/object.h"

namespace gel {
auto PointerVisitor::Visit(Object* ptr) -> bool {
  ASSERT(ptr);
  return Visit(ptr->raw_ptr());
}

auto Pointer::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  return GetObjectPointer()->VisitPointers(vis);
}

auto Pointer::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  return GetObjectPointer()->VisitPointerPointers(vis);
}
}  // namespace gel