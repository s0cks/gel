#ifndef GEL_INVOKE_INSTANCE_EXPR_H
#define GEL_INVOKE_INSTANCE_EXPR_H

#include "gel/expr/invoke_expr.h"

namespace gel::expr {
class InvokeInstanceExpr : public TemplateInvokeExpr<Fn> {
 private:
  explicit InvokeInstanceExpr(Fn* target, Expression* instance, const ExpressionList& args) :
    TemplateInvokeExpr(target, {instance}) {
    AddArgs(args);
  }

  inline void SetInstance(Expression* rhs) {
    ASSERT(rhs);
    SetArgAt(0, rhs);
  }

 public:
  ~InvokeInstanceExpr() override = default;

  auto GetInstance() const -> Expression* {
    return GetArgAt(0);
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return GetNumberOfArgs();
  }

  auto VisitChildren(ExpressionVisitor& vis) -> bool override;
  DECLARE_EXPRESSION(InvokeInstanceExpr);

 public:
  static inline auto New(Fn* target, Expression* instance, const ExpressionList& args) -> InvokeInstanceExpr* {
    ASSERT(target);
    ASSERT(instance);
    return new InvokeInstanceExpr(target, instance, args);
  }
};
}  // namespace gel::expr

#endif  // GEL_INVOKE_INSTANCE_EXPR_H
