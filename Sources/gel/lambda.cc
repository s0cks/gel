#include "gel/lambda.h"

#include <glog/logging.h>

#include <ranges>
#include <sstream>

#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/flow_graph_builder.h"
#include "gel/local_scope.h"
#include "gel/pointer.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"

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

auto Lambda::HashCode() const -> uword {
  return Procedure::HashCode();
}

auto Lambda::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Procedure::VisitPointers(vis))
    return false;
  if (HasOwner()) {
    if (!vis->Visit(GetOwner()))
      return false;
  }
  if (HasDocs()) {
    if (!vis->Visit(GetDocs()))
      return false;
  }
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
  ToStringHelper<Lambda> helper;
  if (HasSymbol())
    helper.AddField("symbol", GetSymbol()->GetFullyQualifiedName());
  if (HasOwner())
    helper.AddField("owner", GetOwner());
  helper.AddField("args", GetArgs());
  helper.AddField("empty", IsEmpty());
  if (HasDocs())
    helper.AddField("docs", GetDocs());
  return helper;
}
}  // namespace gel