#include "gel/lambda.h"

#include <glog/logging.h>

#include <ranges>
#include <sstream>

#include "gel/common.h"
#include "gel/expression.h"
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
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Lambda::New(const ObjectList& args) -> Lambda* {
  NOT_IMPLEMENTED(FATAL);
}

auto Lambda::ToString() const -> std::string {
  ToStringHelper<Lambda> helper;
  if (HasSymbol())
    helper.AddField("name", GetSymbol()->Get());
  if (HasOwner())
    helper.AddField("owner", GetOwner());
  helper.AddField("args", GetArgs());
  helper.AddField("empty", IsEmpty());
  if (HasDocstring())
    helper.AddField("docs", GetDocstring());
  return helper;
}
}  // namespace gel