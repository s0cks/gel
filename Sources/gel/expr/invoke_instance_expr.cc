#include "gel/expr/invoke_instance_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto InvokeInstanceExpr::ToString() const -> std::string {
  ToStringHelper<InvokeExpr> helper{};
  helper.AddField("target", GetTarget());
  helper.AddField("instance", GetInstance());
  helper.AddField("num_args", GetNumberOfArgs());
  helper.AddField("args", GetArgs());
  return helper;
}

auto InvokeInstanceExpr::VisitChildren(ExpressionVisitor& vis) -> bool {
  return VisitArgs(vis);
}
}  // namespace gel::expr