#ifndef GEL_INVOKE_NATIVE_EXPR_H
#define GEL_INVOKE_NATIVE_EXPR_H

#include "gel/expr/invoke_expr.h"

namespace gel::expr {
class InvokeNativeExpr : public TemplateInvokeExpr<NativeFn> {
 private:
  explicit InvokeNativeExpr(NativeFn* target, const ExpressionList& args) :
    TemplateInvokeExpr<NativeFn>(target, args) {}

  void SetChildAt(const uint64_t idx, Expression* expr) override {
    return SetArgAt(idx, expr);
  }

 public:
  ~InvokeNativeExpr() override = default;

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    return GetArgAt(idx);
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return GetNumberOfArgs();
  }

  auto VisitChildren(ExpressionVisitor& vis) -> bool override;
  DECLARE_EXPRESSION(InvokeNativeExpr);

 public:
  static inline auto New(NativeFn* target, const ExpressionList& args) -> InvokeNativeExpr* {
    ASSERT(target);
    return new InvokeNativeExpr(target, args);
  }
};
}  // namespace gel::expr

#endif  // GEL_INVOKE_NATIVE_EXPR_H
