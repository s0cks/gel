#ifndef GEL_WHEN_EXPR_H
#define GEL_WHEN_EXPR_H

#include "gel/frontend/expr/expr.h"

namespace gel::expr {
class WhenExpr : public Expression {
  friend class gel::MacroEffectVisitor;

 private:
  Expression* test_;
  ExpressionList actions_;

 protected:
  WhenExpr(Expression* test, const ExpressionList& actions) :
    Expression(),
    test_(test),
    actions_(actions) {
    ASSERT(test_);
    ASSERT(!actions_.empty());
  }

  void SetTest(Expression* test) {
    ASSERT(test);
    test_ = test;
  }

  void SetActions(const ExpressionList& actions) {
    ASSERT(!actions.empty());
    actions_ = actions;
  }

  void SetActionAt(const uint64_t idx, Expression* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfActions());
    actions_[idx] = expr;
  }

 public:
  ~WhenExpr() override = default;

  auto GetTest() const -> Expression* {
    return test_;
  }

  auto GetActions() const -> const ExpressionList& {
    return actions_;
  }

  auto GetNumberOfActions() const -> uint64_t {
    return actions_.size();
  }

  auto GetActionAt(const uint64_t idx) const -> Expression* {
    ASSERT(idx >= 0 && idx <= GetNumberOfActions());
    return actions_[idx];
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return 1 + GetNumberOfActions();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return idx == 0 ? GetTest() : GetActionAt(idx - 1);
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(WhenExpr);

 public:
  static inline auto New(Expression* test, const ExpressionList& actions = {}) -> WhenExpr* {
    ASSERT(test);
    ASSERT(!actions.empty());
    return new WhenExpr(test, actions);
  }

  static inline auto New(Expression* test, Expression* action) -> WhenExpr* {
    ASSERT(test);
    ASSERT(action);
    return New(test, ExpressionList{action});
  }
};
}  // namespace gel::expr

#endif  // GEL_WHEN_EXPR_H
