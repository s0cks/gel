#include "expr/foreach_expr.h"

#include "to_string_helper.h"

namespace gel::expr {
auto ForeachExpr::ToString() const -> std::string {
  ToStringHelper<ForeachExpr> helper{};
  helper.AddArrayField("bindings", GetBindings());
  helper.AddField("body", GetBody());
  return helper;
}
}  // namespace gel::expr
