#include "expr/binary_expr.h"

#include "to_string_helper.h"

namespace gel::expr {
auto BinaryOpExpr::IsConstantExpr() const -> bool {
  return GetLeft()->IsConstantExpr() && GetRight()->IsConstantExpr();
}

auto BinaryOpExpr::EvalToConstant(LocalScope* scope) const -> Object* {
  ASSERT(scope);
  ASSERT(IsConstantExpr());
  const auto left = GetLeft()->EvalToConstant(scope);
  ASSERT(left);
  const auto right = GetRight()->EvalToConstant(scope);
  ASSERT(right);
  switch (GetOp()) {
#define DEFINE_BINARY_OP_CASE(Name) \
  case BinaryOp::k##Name:           \
    return left->Name(right);
    FOR_EACH_BINARY_OP(DEFINE_BINARY_OP_CASE)
#undef DEFINE_BINARY_OP_CASE
    default:
      LOG(FATAL) << "invalid binary op: " << GetOp();
      return nullptr;
  }
}

auto BinaryOpExpr::VisitChildren(ExpressionVisitor& vis) -> bool {
  if (!GetLeft()->Accept(vis))
    return false;
  if (!GetRight()->Accept(vis))
    return false;
  return true;
}

auto BinaryOpExpr::ToString() const -> std::string {
  ToStringHelper<BinaryOpExpr> helper{};
  helper.AddField("op", GetOp());
  helper.AddField("left", GetLeft());
  helper.AddField("right", GetRight());
  return helper;
}
}  // namespace gel::expr
