#include "gel/expr/store_field_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto StoreFieldExpr::ToString() const -> std::string {
  ToStringHelper<StoreFieldExpr> helper{};
  helper.AddField("field", GetField());
  helper.AddField("value", GetValue());
  return helper;
}
}  // namespace gel::expr