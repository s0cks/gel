#include "scheme/expression.h"

#include <glog/logging.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "scheme/common.h"

namespace scm::expr {
#define DEFINE_ACCEPT(Name)                                 \
  auto Name##Expr::Accept(ExpressionVisitor* vis) -> bool { \
    ASSERT(vis);                                            \
    return vis->Visit##Name(this);                          \
  }
FOR_EACH_EXPRESSION_NODE(DEFINE_ACCEPT)
#undef DEFINE_ACCEPT

auto SequenceExpr::IsConstantExpr() const -> bool {
  const auto found = std::ranges::find_if(std::begin(children_), std::end(children_), [](Expression* expr) {
    return !expr->IsConstantExpr();
  });
  return found != std::end(children_);
}

auto SequenceExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& expr : children_) {
    if (!expr->Accept(vis))
      return false;
  }
  return true;
}

auto LiteralExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "LiteralExpr(";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto BinaryOpExpr::IsConstantExpr() const -> bool {
  return GetLeft()->IsConstantExpr() && GetRight()->IsConstantExpr();
}

auto BinaryOpExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  if (!GetLeft()->Accept(vis))
    return false;
  if (!GetRight()->Accept(vis))
    return false;
  return true;
}

auto BinaryOpExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "BinaryOpExpr(";
  ss << "op=" << GetOp() << ", ";
  ss << "left=" << GetLeft()->ToString() << ", ";
  ss << "right=" << GetRight()->ToString();
  ss << ")";
  return ss.str();
}

auto EvalExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "EvalExpr(";
  if (HasExpression())
    ss << "expr=" << GetExpression()->ToString();
  ss << ")";
  return ss.str();
}

auto BeginExpr::ToString() const -> std::string {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  std::stringstream ss;
  ss << "BeginExpr(";
  ss << ")";
  return ss.str();
}

auto DefineExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "DefineExpr(";
  ss << "symbol=" << GetSymbol() << ", ";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto CallProcExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "CallProcExpr(";
  ss << "symbol=" << GetSymbol();
  ss << ")";
  return ss.str();
}

auto SymbolExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "SymbolExpr(";
  ss << "symbol=" << GetSymbol();
  ss << ")";
  return ss.str();
}
}  // namespace scm::expr