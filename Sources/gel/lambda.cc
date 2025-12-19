#include "lambda.h"

#include <glog/logging.h>
#include <string>

#include "common.h"
#include "local_scope.h"
#include "platform.h"
#include "pointer.h"
#include "to_string_helper.h"
#include "type.h"

namespace gel {
auto Lambda::CreateClass() -> Class* {
  return Class::New(Procedure::GetClass(), kClassName);
}

auto Lambda::Equals(Object* rhs) const -> bool {
  if (!rhs->IsLambda())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto Lambda::GetHashCode() const -> HashCode {
  return Procedure::GetHashCode();
}

auto Lambda::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Lambda::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Procedure::VisitPointers(vis))
    return false;
  if (!Visit(body_, *vis))
    return false;
  // TODO: visit code_
  return true;
}

auto Lambda::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Procedure::VisitPointerPointers(vis))
    return false;
  // TODO: visit body_
  return true;
}

auto Lambda::New(const ObjectList& args) -> Lambda* {
  NOT_IMPLEMENTED(FATAL);
}

auto Lambda::ToString() const -> std::string {
  ToStringHelper<Lambda> helper{};
  if (HasSymbol())
    helper.AddField("symbol", GetSymbol()->GetFullyQualifiedName());
  helper.AddField("owner", GetOwner());
  helper.AddField("args", GetArgs());
  helper.AddField("empty", IsEmpty());
  helper.AddField("docs", GetDocstring());
  return helper;
}
}  // namespace gel
