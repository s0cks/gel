#ifndef GEL_STORE_LOCAL_EXPR_H
#define GEL_STORE_LOCAL_EXPR_H

#include "gel/expr/expression.h"

namespace gel::expr {
class StoreLocalExpr : public Expression {
  friend class gel::MacroEffectVisitor;

 private:
  LocalVariable* local_;
  Expression* value_;

 protected:
  StoreLocalExpr(LocalVariable* local, Expression* value) :
    Expression(),
    local_(local),
    value_(value) {
    ASSERT(local_);
  }

  void SetValue(Expression* rhs) {
    ASSERT(rhs);
    value_ = rhs;
  }

 public:
  ~StoreLocalExpr() override = default;

  auto GetLocal() const -> LocalVariable* {
    return local_;
  }

  auto GetValue() const -> Expression* {
    return value_;
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  auto VisitChildren(ExpressionVisitor& vis) -> bool override {
    if (!HasValue())
      return false;
    return GetValue()->Accept(vis);
  }

  DECLARE_EXPRESSION(StoreLocalExpr);

 public:
  static inline auto New(LocalVariable* local, Expression* value) -> StoreLocalExpr* {
    ASSERT(local);
    return new StoreLocalExpr(local, value);
  }
};
}  // namespace gel::expr

#endif  // GEL_STORE_LOCAL_EXPR_H
