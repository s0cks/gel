#ifndef GEL_COND_EXPR_H
#define GEL_COND_EXPR_H

#include "expr/clause_expr.h"

namespace gel::expr {
class CondExpr : public Expression {
 private:
  ClauseList clauses_;
  Expression* alt_ = nullptr;

 protected:
  CondExpr(const ClauseList& clauses, Expression* alt) :
    Expression(),
    clauses_(clauses),
    alt_(alt) {
    ASSERT(!clauses_.empty());
  }

 public:
  ~CondExpr() override = default;

  auto GetClauses() const -> const ClauseList& {
    return clauses_;
  }

  auto GetNumberOfClauses() const -> uint64_t {
    return clauses_.size();
  }

  auto GetClauseAt(const uint64_t idx) const -> ClauseExpr* {
    ASSERT(idx >= 0 && idx <= GetNumberOfClauses());
    return clauses_[idx];
  }

  void SetClauseAt(const uint64_t idx, ClauseExpr* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfClauses());
    clauses_[idx] = expr;
  }

  auto GetAlternate() const -> Expression* {
    return alt_;
  }

  inline auto HasAlternate() const -> bool {
    return GetAlternate() != nullptr;
  }

  inline void SetAlt(Expression* expr) {
    ASSERT(expr);
    alt_ = expr;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return (HasAlternate() ? 1 : 0) + GetNumberOfClauses();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx < GetNumberOfChildren());
    return (idx >= 0 && idx <= GetNumberOfClauses()) ? clauses_[idx] : GetAlternate();
  }

  auto VisitAllClauses(ExpressionVisitor& vis) -> bool;
  auto VisitChildren(ExpressionVisitor& vis) -> bool override;
  DECLARE_EXPRESSION(CondExpr);

 public:
  static inline auto New(const ClauseList& clauses = {}, Expression* alt = nullptr) -> CondExpr* {
    ASSERT(!clauses.empty());
    return new CondExpr(clauses, alt);
  }

  static inline auto New(Expression* test, Expression* conseq, Expression* alt = nullptr) -> CondExpr* {
    ASSERT(test);
    ASSERT(conseq);
    return New(
        ClauseList{
            ClauseExpr::New(test, conseq),
        },
        alt);
  }
};
}  // namespace gel::expr

#endif  // GEL_COND_EXPR_H
