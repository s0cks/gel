#include "gel/frontend/expr/unary_expr.h"

#include "gel/to_string_helper.h"

namespace gel::expr {
auto UnaryOpExpr::IsConstantExpr() const -> bool {
  return HasValue() && GetValue()->IsConstantExpr();
}

auto UnaryOpExpr::EvalToConstant(LocalScope* scope) const -> Object* {
  ASSERT(IsConstantExpr());
  const auto value = GetValue()->EvalToConstant(scope);
  switch (GetOp()) {
    case UnaryOp::kCar:
      return gel::Car(value);
    case UnaryOp::kCdr:
      return gel::Cdr(value);
    case UnaryOp::kNot:
      return Bool::Box(gel::Truth(value))->Negate();
    case UnaryOp::kNull:
      return Bool::Box(value->IsNil());
    case UnaryOp::kNonnull:
      return Bool::Box(!value->IsNil());
    case UnaryOp::kBitNot:
      if (!gel::IsNumber(value))
        return gel::Nil::Get();
      return value->AsNumber()->BitNot();
    default:
      LOG(FATAL) << "invalid UnaryOp: " << GetOp();
  }
}

auto UnaryOpExpr::ToString() const -> std::string {
  ToStringHelper<UnaryOpExpr> helper{};
  helper.AddField("op", GetOp());
  helper.AddField("value", GetValue());
  return helper;
}
}  // namespace gel::expr
