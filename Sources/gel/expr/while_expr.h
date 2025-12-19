#ifndef GEL_WHILE_EXPR_H
#define GEL_WHILE_EXPR_H

#include "expr/expression.h"

namespace gel::expr {
class WhileExpr : public Expression {
 private:
  Expression* test_;
  SeqExpr* body_;

 protected:
  explicit WhileExpr(Expression* test, SeqExpr* body) :
    Expression(),
    test_(test),
    body_(body) {
    ASSERT(test_);
    ASSERT(body_);
  }

 public:
  ~WhileExpr() override = default;

  auto GetTest() const -> Expression* {
    return test_;
  }

  auto GetBody() const -> SeqExpr* {
    return body_;
  }

  DECLARE_EXPRESSION(WhileExpr);

 public:
  static inline auto New(Expression* test, SeqExpr* body) -> WhileExpr* {
    return new WhileExpr(test, body);
  }
};
}  // namespace gel::expr

#endif  // GEL_WHILE_EXPR_H
