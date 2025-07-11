#include "gel/expr/binding_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto BindingExpr::ToString() const -> std::string {
  ToStringHelper<BindingExpr> helper{};
  helper.AddField("local", GetLocal());
  helper.AddField("value", GetValue());
  return helper;
}
}  // namespace gel::expr