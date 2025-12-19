#include "expr/seq_expr.h"

#include "to_string_helper.h"

namespace gel::expr {
auto SeqExpr::ToString() const -> std::string {
  ToStringHelper<SeqExpr> helper{};
  helper.AddField("num_children", GetNumberOfChildren());
  return helper;
}

auto SeqExpr::IsConstantExpr() const -> bool {
  const auto found = std::ranges::find_if(std::begin(children_), std::end(children_), [](Expression* expr) {
    return !expr->IsConstantExpr();
  });
  return found != std::end(children_);
}

auto SeqExpr::VisitChildren(ExpressionVisitor& vis) -> bool {
  for (const auto& expr : children_) {
    if (!expr->Accept(vis))
      return false;
  }
  return true;
}

auto SeqExpr::VisitAllDefinitions(ExpressionVisitor& vis) -> bool {
  for (const auto& expr : children_) {
    if (expr->IsDefinition()) {
      if (!expr->Accept(vis))
        return false;
    }
  }
  return true;
}
}  // namespace gel::expr
