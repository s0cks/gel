#ifndef GEL_STORE_FIELD_EXPR_H
#define GEL_STORE_FIELD_EXPR_H

#include "gel/frontend/expr/expr.h"

namespace gel::expr {
class StoreFieldExpr : public Expression {
  friend class gel::MacroEffectVisitor;

 private:
  Field* field_;
  Expression* instance_;
  Expression* value_;

 protected:
  StoreFieldExpr(Field* field, Expression* instance, Expression* value) :
    Expression(),
    field_(field),
    instance_(instance),
    value_(value) {
    ASSERT(field_);
    ASSERT(instance_);
    ASSERT(value_);
  }

  void SetValue(Expression* rhs) {
    ASSERT(rhs);
    value_ = rhs;
  }

 public:
  ~StoreFieldExpr() override = default;

  auto GetField() const -> Field* {
    return field_;
  }

  auto GetInstance() const -> Expression* {
    return instance_;
  }

  inline auto HasInstance() const -> bool {
    return GetInstance() != nullptr;
  }

  auto GetValue() const -> Expression* {
    return value_;
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  auto VisitChildren(ExpressionVisitor& vis) -> bool override {
    if (HasInstance() && !GetInstance()->Accept(vis))
      return false;
    if (!HasValue() && !GetValue()->Accept(vis))
      return false;
    return true;
  }

  DECLARE_EXPRESSION(StoreFieldExpr);

 public:
  static inline auto New(Field* field, Expression* instance, Expression* value) -> StoreFieldExpr* {
    ASSERT(field);
    return new StoreFieldExpr(field, instance, value);
  }
};
}  // namespace gel::expr

#endif  // GEL_STORE_FIELD_EXPR_H
