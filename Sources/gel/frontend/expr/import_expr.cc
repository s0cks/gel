#include "gel/frontend/expr/import_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto ImportExpr::ToString() const -> std::string {
  ToStringHelper<ImportExpr> helper{};
  helper.AddField("module", GetModule());
  return helper;
}
}  // namespace gel::expr
