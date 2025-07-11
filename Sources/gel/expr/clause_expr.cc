#include "gel/expr/clause_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto ClauseExpr::ToString() const -> std::string {
  ToStringHelper<ClauseExpr> helper{};
  helper.AddField("key", GetKey());
  helper.AddField("body", GetBody());
  return helper;
}

auto ClauseExpr::VisitAllActions(ExpressionVisitor& vis) -> bool {
  return GetBody()->VisitChildren(vis);
}

auto ClauseExpr::VisitChildren(ExpressionVisitor& vis) -> bool {
  if (!GetKey()->Accept(vis))
    return false;
  return VisitAllActions(vis);
}
}  // namespace gel::expr