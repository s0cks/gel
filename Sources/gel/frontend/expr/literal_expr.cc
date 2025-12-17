#include "gel/frontend/expr/literal_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto LiteralExpr::ToString() const -> std::string {
  ToStringHelper<LiteralExpr> helper{};
  helper.AddField("value", GetValue());
  return helper;
}
}  // namespace gel::expr
