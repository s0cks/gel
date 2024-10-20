#include "scheme/program.h"

#include <sstream>

#include "scheme/expression.h"

namespace scm {
auto Program::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Program(";
  ss << ")";
  return ss.str();
}

auto Program::Accept(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& expr : expressions_) {
    if (!expr->Accept(vis))
      return false;
  }
  return true;
}

auto Program::VisitExpressions(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& expr : expressions_) {
    if (!expr->Accept(vis))
      return false;
  }
  return true;
}
}  // namespace scm