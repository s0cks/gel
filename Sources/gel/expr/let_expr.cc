#include "expr/let_expr.h"

#include "to_string_helper.h"

namespace gel::expr {
auto LetExpr::VisitAllBindings(ExpressionVisitor& vis) -> bool {
  for (const auto& binding : bindings_) {
    ASSERT(binding);
    if (!binding->Accept(vis))
      return false;
  }
  return true;
}

auto LetExpr::VisitChildren(ExpressionVisitor& vis) -> bool {
  if (!VisitAllBindings(vis))
    return false;
  return GetBody()->Accept(vis);
}

auto LetExpr::ToString() const -> std::string {
  ToStringHelper<LetExpr> helper{};
  helper.AddField("scope", GetScope());
  helper.AddField("binings", GetBindings());
  helper.AddField("body", GetBody());
  return helper;
}

}  // namespace gel::expr
