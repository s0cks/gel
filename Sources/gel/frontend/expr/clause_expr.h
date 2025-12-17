#ifndef GEL_CLAUSE_EXPR_H
#define GEL_CLAUSE_EXPR_H

#include "gel/frontend/expr/seq_expr.h"

namespace gel::expr {
class ClauseExpr : public Expression {  // TODO: should this be a WhenExpr?
 private:
  Expression* key_;
  Expression* body_;

  ClauseExpr(Expression* key, Expression* body) :
    Expression(),
    key_(key),
    body_(body) {
    ASSERT(key_);
    ASSERT(body_);
  }

 public:
  ~ClauseExpr() override = default;

  auto GetKey() const -> Expression* {
    return key_;
  }

  auto GetBody() const -> Expression* {
    return body_;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return 2;
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return idx == 0 ? GetKey() : idx == 1 ? GetBody() : nullptr;
  }

  auto VisitAllActions(ExpressionVisitor& vis) -> bool;
  auto VisitChildren(ExpressionVisitor& vis) -> bool override;
  DECLARE_EXPRESSION(ClauseExpr);

 public:
  static inline auto New(Expression* key, Expression* body) -> ClauseExpr* {
    ASSERT(key);
    ASSERT(body);
    return new ClauseExpr(key, body);
  }

  static inline auto New(Expression* key, const ExpressionList& body) -> ClauseExpr* {
    return New(key, SeqExpr::New(body));
  }
};

using ClauseList = std::vector<ClauseExpr*>;

static inline auto operator<<(std::ostream& stream, const ClauseList& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& clause : rhs) {
    stream << clause->ToString();
    if (--remaining >= 1)
      stream << ", ";
  }
  stream << "]";
  return stream;
}
}  // namespace gel::expr

#endif  // GEL_CLAUSE_EXPR_H
