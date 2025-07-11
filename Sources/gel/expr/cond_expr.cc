#include "gel/expr/cond_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto CondExpr::VisitAllClauses(ExpressionVisitor& vis) -> bool {
  for (const auto& clause : clauses_) {
    ASSERT(clause);
    if (!clause->Accept(vis))
      return false;
  }
  return true;
}

auto CondExpr::VisitChildren(ExpressionVisitor& vis) -> bool {
  if (!VisitAllClauses(vis))
    return false;
  if (HasAlternate()) {
    if (!GetAlternate()->Accept(vis))
      return false;
  }
  return true;
}

auto CondExpr::ToString() const -> std::string {
  ToStringHelper<CondExpr> helper{};
  helper.AddField("clauses", GetClauses());
  helper.AddField("alternate", GetAlternate());
  return helper;
}
}  // namespace gel::expr