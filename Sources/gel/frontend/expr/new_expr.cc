#include "gel/frontend/expr/new_expr.h"

#include "gel/frontend/expr/literal_expr.h"
#include "gel/to_string_helper.h"

namespace gel::expr {

auto NewExpr::ToString() const -> std::string {
  ToStringHelper<NewExpr> helper{};
  helper.AddField("target", GetTargetClass());
  helper.AddField("args", GetArgs());
  return helper;
}

auto NewExpr::VisitArgs(ExpressionVisitor& vis) -> bool {
  for (const auto& arg : args_) {
    if (!arg->Accept(vis))
      return false;
  }
  return true;
}

auto NewExpr::IsConstantExpr() const -> bool {
  return false;
}

auto NewExpr::EvalToConstant(LocalScope* scope) const -> Object* {
  ASSERT(scope);
  ObjectList values{};
  for (const auto& arg : args_) {
    if (!arg->IsConstantExpr()) {
      return nullptr;
    } else if (expr::IsLiteralSymbol(arg) && scope) {
      const auto literal = arg->AsLiteralExpr()->GetValue()->AsSymbol();
      ASSERT(literal);
      LocalVariable* local = nullptr;
      if (!scope->Lookup(literal, &local))
        return nullptr;
      if (!local || !local->HasValue())
        return nullptr;
      values.push_back(local->GetValue());
      continue;
    }
    values.push_back(arg->EvalToConstant(scope));
  }
  return GetTargetClass()->NewInstance(values);
}
}  // namespace gel::expr
