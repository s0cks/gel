#include "gel/error.h"

#include <sstream>

#include "gel/common.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/to_string_helper.h"

namespace gel {
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

auto Error::New(const ObjectList& args) -> Error* {
  if (args.empty())
    return new Error(String::New());
  return New(args[0]);
}

auto Error::ToString() const -> std::string {
  ToStringHelper<Error> helper;
  helper.AddField("message", GetMessage()->Get());
  return helper;
}
}  // namespace gel