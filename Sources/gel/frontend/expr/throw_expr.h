#ifndef GEL_THROW_EXPR_H
#define GEL_THROW_EXPR_H

#include "gel/frontend/expr/expr.h"

namespace gel::expr {
class ThrowExpr : public TemplateExpression<1> {
 protected:
  explicit ThrowExpr(Expression* value) {
    SetValue(value);
  }

  inline void SetValue(Expression* expr) {
    ASSERT(expr);
    SetChildAt(0, expr);
  }

 public:
  ~ThrowExpr() override = default;

  inline auto GetValue() const -> Expression* {
    return GetChildAt(0);
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  DECLARE_EXPRESSION(ThrowExpr);

 public:
  static inline auto New(Expression* value) -> ThrowExpr* {
    ASSERT(value);
    return new ThrowExpr(value);
  }
};
}  // namespace gel::expr

#endif  // GEL_THROW_EXPR_H
