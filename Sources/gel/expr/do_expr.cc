#include "expr/do_expr.h"

#include "to_string_helper.h"

namespace gel::expr {
auto DoExpr::ToString() const -> std::string {
  ToStringHelper<DoExpr> helper{};
  helper.AddField("body", GetBody());
  return helper;
}
}  // namespace gel::expr
