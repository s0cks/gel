#include "gel/frontend/expr/while_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto WhileExpr::ToString() const -> std::string {
  ToStringHelper<WhileExpr> helper{};
  helper.AddField("test", GetTest());
  helper.AddField("body", GetBody());
  return helper;
}
}  // namespace gel::expr
