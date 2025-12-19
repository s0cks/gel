#include "expr/throw_expr.h"

#include "to_string_helper.h"

namespace gel::expr {
auto ThrowExpr::ToString() const -> std::string {
  ToStringHelper<ThrowExpr> helper{};
  helper.AddField("value", GetValue());
  return helper;
}
}  // namespace gel::expr
