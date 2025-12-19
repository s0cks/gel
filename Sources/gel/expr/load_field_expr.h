#ifndef GEL_LOAD_FIELD_EXPR_H
#define GEL_LOAD_FIELD_EXPR_H

#include "expr/expression.h"

namespace gel::expr {
class LoadFieldExpr : public TemplateExpression<1> {
 private:
  Field* field_;

  LoadFieldExpr(Expression* instance, Field* field) :
    TemplateExpression(),
    field_(field) {
    SetInstance(instance);
    ASSERT(field_);
    ASSERT(HasInstance());
  }

  void SetInstance(Expression* expr) {
    ASSERT(expr);
    SetChildAt(0, expr);
  }

 public:
  ~LoadFieldExpr() override = default;

  auto GetField() const -> Field* {
    return field_;
  }

  inline auto GetInstance() const -> Expression* {
    return GetChildAt(0);
  }

  inline auto HasInstance() const -> bool {
    return GetInstance() != nullptr;
  }

  DECLARE_EXPRESSION(LoadFieldExpr);

 public:
  static inline auto New(Expression* instance, Field* field) -> LoadFieldExpr* {
    ASSERT(instance);
    ASSERT(field);
    return new LoadFieldExpr(instance, field);
  }
};
}  // namespace gel::expr

#endif  // GEL_LOAD_FIELD_EXPR_H
