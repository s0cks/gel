#ifndef GEL_NEW_EXPR_H
#define GEL_NEW_EXPR_H

#include "gel/expr/expression.h"

namespace gel::expr {
class NewExpr : public Expression {
 private:
  Class* target_;
  ExpressionList args_;

 protected:
  NewExpr(Class* target, const ExpressionList& args) :
    target_(target),
    args_(args) {}

 public:
  ~NewExpr() override = default;

  auto GetTargetClass() const -> Class* {
    return target_;
  }

  auto GetArgs() const -> const ExpressionList& {
    return args_;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return args_.size();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return args_[idx];
  }

  auto VisitArgs(ExpressionVisitor& vis) -> bool;

  auto VisitChildren(ExpressionVisitor& vis) -> bool override {
    return VisitArgs(vis);
  }

  auto EvalToConstant(LocalScope* scope) const -> Object* override;
  auto IsConstantExpr() const -> bool override;
  DECLARE_EXPRESSION(NewExpr);

 public:
  static inline auto New(Class* target, const ExpressionList& args) -> NewExpr* {
    return new NewExpr(target, args);
  }
};
}  // namespace gel::expr

#endif  // GEL_NEW_EXPR_H
