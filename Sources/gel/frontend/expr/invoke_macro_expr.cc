#include "gel/frontend/expr/invoke_macro_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto InvokeMacroExpr::ToString() const -> std::string {
  ToStringHelper<InvokeMacroExpr> helper{};
  helper.AddField("target", GetTarget());
  helper.AddField("num_args", GetNumberOfArgs());
  return helper;
}
}  // namespace gel::expr
