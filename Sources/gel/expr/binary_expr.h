#ifndef GEL_BINARY_EXPR_H
#define GEL_BINARY_EXPR_H

#include "gel/binary_op.h"
#include "gel/expr/expression.h"

namespace gel::expr {
class BinaryOpExpr : public TemplateOpExpression<BinaryOp, 2> {
  static constexpr const auto kLeftInput = 0;
  static constexpr const auto kRightInput = 1;

 protected:
  explicit BinaryOpExpr(const BinaryOp op, Expression* left, Expression* right) :
    TemplateOpExpression<BinaryOp, 2>(op) {
    SetLeft(left);
    SetRight(right);
  }

 public:
  ~BinaryOpExpr() override = default;

  inline auto GetLeft() const -> Expression* {
    return GetChildAt(kLeftInput);
  }

  inline auto HasLeft() const -> bool {
    return GetLeft() != nullptr;
  }

  inline void SetLeft(Expression* value) {
    ASSERT(value);
    SetChildAt(kLeftInput, value);
  }

  auto GetRight() const -> Expression* {
    return GetChildAt(kRightInput);
  }

  inline auto HasRight() const -> bool {
    return GetRight() != nullptr;
  }

  inline void SetRight(Expression* value) {
    ASSERT(value);
    SetChildAt(kRightInput, value);
  }

#define DEFINE_OP_CHECK(Name)              \
  inline auto Is##Name##Op() const->bool { \
    return GetOp() == BinaryOp::k##Name;   \
  }
  FOR_EACH_BINARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK

  auto IsConstantExpr() const -> bool override;
  auto EvalToConstant(LocalScope* scope) const -> Object* override;
  auto VisitChildren(ExpressionVisitor& vis) -> bool override;
  DECLARE_EXPRESSION(BinaryOpExpr);

 public:
  static inline auto New(const BinaryOp op, Expression* left, Expression* right) -> BinaryOpExpr* {
    ASSERT(left);
    ASSERT(right);
    return new BinaryOpExpr(op, left, right);
  }

#define DEFINE_NEW_OP(Name)                                                       \
  static inline auto New##Name(Expression* lhs, Expression* rhs)->BinaryOpExpr* { \
    ASSERT(lhs);                                                                  \
    ASSERT(rhs);                                                                  \
    return New(BinaryOp::k##Name, lhs, rhs);                                      \
  }
  FOR_EACH_BINARY_OP(DEFINE_NEW_OP)
#undef DEFINE_NEW_OP
};
}  // namespace gel::expr

#endif  // GEL_BINARY_EXPR_H
