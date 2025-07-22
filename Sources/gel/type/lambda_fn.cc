#include "gel/lambda.h"

#include <glog/logging.h>
#include <string>

#include "gel/common.h"
#include "gel/local_scope.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"

namespace gel {
auto LambdaFnFn::CreateClass() -> Class* {
  return Class::New(Fn::GetClass(), kClassName);
}

auto LambdaFnFn::Equals(Object* rhs) const -> bool {
  if (!rhs->IsLambdaFnFn())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto LambdaFnFn::GetHashCode() const -> HashCode {
  return Fn::GetHashCode();
}

auto LambdaFnFn::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto LambdaFnFn::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Fn::VisitPointers(vis))
    return false;
  if (!Visit(body_, *vis))
    return false;
  // TODO: visit code_
  return true;
}

auto LambdaFnFn::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Fn::VisitPointerPointers(vis))
    return false;
  // TODO: visit body_
  return true;
}

auto LambdaFnFn::New(const ObjectList& args) -> LambdaFnFn* {
  NOT_IMPLEMENTED(FATAL);
}

auto LambdaFnFn::ToString() const -> std::string {
  ToStringHelper<LambdaFnFn> helper{};
  if (HasSymbol())
    helper.AddField("symbol", GetSymbol()->GetFullyQualifiedName());
  helper.AddField("owner", GetOwner());
  helper.AddField("args", GetArgs());
  helper.AddField("empty", IsEmpty());
  helper.AddField("docs", GetDocstring());
  return helper;
}
}  // namespace gel