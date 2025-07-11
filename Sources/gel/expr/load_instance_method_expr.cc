#include "gel/expr/load_instance_method_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto LoadInstanceMethodExpr::IsConstantExpr() const -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto LoadInstanceMethodExpr::EvalToConstant(LocalScope* scope) const -> Object* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return nullptr;
}

auto LoadInstanceMethodExpr::ToString() const -> std::string {
  ToStringHelper<LoadInstanceMethodExpr> helper{};
  helper.AddField("class", GetTargetClass());
  helper.AddField("name", GetTargetName());
  return helper;
}
}  // namespace gel::expr