#ifndef GEL_LITERAL_EXPR_H
#define GEL_LITERAL_EXPR_H

#include "gel/frontend/expr/expr.h"

namespace gel::expr {
class LiteralExpr : public Expression {
 private:
  Object* value_;

 protected:
  explicit LiteralExpr(Object* value) :
    Expression(),
    value_(value) {}

 public:
  ~LiteralExpr() override = default;

  auto GetValue() const -> Object* {
    return value_;
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  auto IsConstantExpr() const -> bool override {
    return !IsLiteralSymbol();
  }

  auto EvalToConstant(LocalScope* scope) const -> Object* override {
    return value_;
  }

  auto IsLiteralSymbol() const -> bool {
    return HasValue() && GetValue()->IsSymbol();
  }

  DECLARE_EXPRESSION(LiteralExpr);

 public:
  static inline auto New(Object* value) -> LiteralExpr* {
    ASSERT(value);
    return new LiteralExpr(value);
  }
};

static inline auto IsLiteralSymbol(Expression* rhs) -> bool {
  if (!rhs || !rhs->IsLiteralExpr())
    return false;
  const auto literal = rhs->AsLiteralExpr()->GetValue();
  return literal && literal->IsSymbol();
}

static inline auto IsLiteralSymbol(Expression* rhs, Symbol* value) -> bool {
  if (!rhs || !rhs->IsLiteralExpr())
    return false;
  const auto literal = rhs->AsLiteralExpr()->GetValue();
  return literal && literal->Equals(value);
}
}  // namespace gel::expr

#endif  // GEL_LITERAL_EXPR_H
