#include "gel/expr/load_field_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto LoadFieldExpr::ToString() const -> std::string {
  ToStringHelper<LoadFieldExpr> helper{};
  helper.AddField("instance", GetInstance());
  helper.AddField("field", GetField());
  return helper;
}
}  // namespace gel::expr