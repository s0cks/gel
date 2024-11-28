#include "scheme/error.h"

#include <sstream>

#include "scheme/common.h"
#include "scheme/object.h"
#include "scheme/pointer.h"

namespace scm {
auto Error::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Error::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsError())
    return false;
  ASSERT(rhs->IsError());
  const auto other = rhs->AsError();
  return GetMessage() == other->GetMessage();
}

auto Error::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  return vis->Visit((*raw_ptr()));
}

auto Error::VisitPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  return vis->Visit(raw_ptr());
}

auto Error::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Error(";
  ss << "message=" << GetMessage();
  ss << ")";
  return ss.str();
}
}  // namespace scm