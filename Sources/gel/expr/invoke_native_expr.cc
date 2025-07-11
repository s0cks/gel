#include "gel/expr/invoke_native_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto InvokeNativeExpr::ToString() const -> std::string {
  ToStringHelper<InvokeExpr> helper{};
  helper.AddField("target", GetTarget());
  helper.AddField("num_args", GetNumberOfArgs());
  helper.AddField("args", GetArgs());
  return helper;
}

auto InvokeNativeExpr::VisitChildren(ExpressionVisitor& vis) -> bool {
  return VisitArgs(vis);
}
}  // namespace gel::expr