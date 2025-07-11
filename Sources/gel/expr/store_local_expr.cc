#include "gel/expr/store_local_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto StoreLocalExpr::ToString() const -> std::string {
  ToStringHelper<StoreLocalExpr> helper{};
  helper.AddField("local", GetLocal());
  helper.AddField("value", GetValue());
  return helper;
}
}  // namespace gel::expr