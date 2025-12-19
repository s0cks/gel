#include "gel/error.h"

#include <glog/logging.h>
#include <string>

#include "gel/common.h"
#include "gel/object.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"

namespace gel {
auto Error::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Error::GetHashCode() const -> HashCode {
  uword hash = 0;
  return hash;
}

auto Error::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
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

auto Error::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  return vis->Visit(raw_ptr());
}

auto Error::New(const ObjectList& args) -> Error* {
  if (args.empty())
    return new Error(String::New());
  return New(args[0]);
}

auto Error::ToString() const -> std::string {
  ToStringHelper<Error> helper{};
  helper.AddField("message", GetMessage()->Get());
  return helper;
}
}  // namespace gel
