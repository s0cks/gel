#ifndef GEL_UNARY_EXPR_H
#define GEL_UNARY_EXPR_H

#include "gel/frontend/expr/expr.h"
#include "gel/frontend/expr/unary_op.h"

namespace gel::expr {
class UnaryOpExpr : public TemplateOpExpression<UnaryOp, 1> {
 protected:
  UnaryOpExpr(const UnaryOp op, Expression* value) :
    TemplateOpExpression<UnaryOp, 1>(op) {
    SetChildAt(0, value);
  }

 public:
  ~UnaryOpExpr() override = default;

  inline auto GetValue() const -> Expression* {
    return GetChildAt(0);
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  inline void SetValue(Expression* expr) {
    ASSERT(expr);
    SetChildAt(0, expr);
  }

#define DEFINE_OP_CHECK(Name)              \
  inline auto Is##Name##Op() const->bool { \
    return GetOp() == UnaryOp::k##Name;    \
  }
  FOR_EACH_UNARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK

  auto IsConstantExpr() const -> bool override;
  auto EvalToConstant(LocalScope* scope) const -> Object* override;
  DECLARE_EXPRESSION(UnaryOpExpr);

 public:
  static inline auto New(const UnaryOp op, Expression* value) -> UnaryOpExpr* {
    ASSERT(value);
    return new UnaryOpExpr(op, value);
  }

#define DEFINE_NEW_OP(Name)                                       \
  static inline auto New##Name(Expression* value)->UnaryOpExpr* { \
    ASSERT(value);                                                \
    return New(UnaryOp::k##Name, value);                          \
  }
  FOR_EACH_UNARY_OP(DEFINE_NEW_OP)
#undef DEFINE_NEW_OP
};
}  // namespace gel::expr

#endif  // GEL_UNARY_EXPR_H
