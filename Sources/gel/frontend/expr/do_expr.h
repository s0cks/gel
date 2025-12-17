#ifndef GEL_DO_EXPR_H
#define GEL_DO_EXPR_H

#include "gel/frontend/expr/expr.h"

namespace gel::expr {
class DoExpr : public Expression {
 private:
  Expression* body_;

 protected:
  explicit DoExpr(Expression* body) :
    Expression(),
    body_(body) {}

  void SetBody(Expression* rhs) {
    ASSERT(rhs);
    body_ = rhs;
  }

  void SetChildAt(const uint64_t idx, Expression* rhs) override {
    ASSERT(rhs);
    ASSERT(idx == 0);
    return SetBody(rhs);
  }

 public:
  ~DoExpr() override = default;

  auto GetBody() const -> Expression* {
    return body_;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return 1;
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx == 0);
    return body_;
  }

  auto VisitChildren(ExpressionVisitor& vis) -> bool override {
    return GetBody()->Accept(vis);
  }

  DECLARE_EXPRESSION(DoExpr);

 public:
  static inline auto New(Expression* body) -> DoExpr* {
    return new DoExpr(body);
  }
};
}  // namespace gel::expr

#endif  // GEL_DO_EXPR_H
