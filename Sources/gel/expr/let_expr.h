#ifndef GEL_LET_EXPR_H
#define GEL_LET_EXPR_H

#include "expr/binding_expr.h"
#include "expr/seq_expr.h"

namespace gel::expr {
class TemplateLetExpr : public SeqExpr {
  DEFINE_NON_COPYABLE_TYPE(TemplateLetExpr);

 private:
  LocalScope* scope_;

 protected:
  explicit TemplateLetExpr(LocalScope* scope, const ExpressionList& body) :
    SeqExpr(body),
    scope_(scope) {
    ASSERT(scope_);
  }

 public:
  ~TemplateLetExpr() override = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }
};

class LetExpr : public Expression {
 private:
  LocalScope* scope_;
  BindingList bindings_;
  SeqExpr* body_;

 protected:
  LetExpr(LocalScope* scope, const BindingList& bindings, SeqExpr* body) :
    Expression(),
    scope_(scope),
    bindings_(bindings),
    body_(body) {
    ASSERT(scope_);
    ASSERT(body_);
  }

 public:
  ~LetExpr() override = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetBindings() const -> const BindingList& {
    return bindings_;
  }

  auto GetNumberOfBindings() const -> uword {
    return bindings_.size();
  }

  inline auto HasBindings() const -> bool {
    return GetNumberOfBindings() > 0;
  }

  auto GetBindingAt(const uword idx) const -> BindingExpr* {
    ASSERT(idx >= 0 && idx <= GetNumberOfBindings());
    return bindings_[idx];
  }

  void SetBindingAt(const uword idx, BindingExpr* rhs) {
    ASSERT(rhs);
    ASSERT(idx >= 0 && idx <= GetNumberOfBindings());
    bindings_[idx] = rhs;
  }

  auto GetBody() const -> SeqExpr* {
    return body_;
  }

  auto IsConstantExpr() const -> bool override {
    return false;
  }

  auto VisitAllBindings(ExpressionVisitor& vis) -> bool;
  auto VisitChildren(ExpressionVisitor& vis) -> bool override;
  DECLARE_EXPRESSION(LetExpr);

 public:
  static inline auto New(LocalScope* scope, const BindingList& bindings = {}, SeqExpr* body = SeqExpr::New())
      -> LetExpr* {
    ASSERT(scope);
    return new LetExpr(scope, bindings, body);
  }
};
}  // namespace gel::expr

#endif  // GEL_LET_EXPR_H
