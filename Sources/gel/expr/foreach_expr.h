#ifndef GEL_FOREACH_EXPR_H
#define GEL_FOREACH_EXPR_H

#include "expr/binding_expr.h"
#include "expr/expression.h"

namespace gel::expr {
class ForeachExpr : public Expression {
 private:
  BindingList bindings_{};

  inline void SetBody(Expression* rhs) {
    ASSERT(rhs);
    SetChildAt(1, rhs);
  }

 public:
  ForeachExpr(const BindingList bindings, Expression* body) :
    Expression(),
    bindings_(std::move(bindings)) {
    SetBody(body);
  }
  ~ForeachExpr() override = default;

  auto GetBindings() const -> const BindingList& {
    return bindings_;
  }

  auto GetNumberOfBindings() const -> uword {
    return bindings_.size();
  }

  auto GetBindingAt(const uword idx) const -> BindingExpr* {
    return bindings_.at(idx);
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return GetNumberOfBindings() + 1;
  }

  auto GetBody() const -> Expression* {
    return GetChildAt(1);
  }

  DECLARE_EXPRESSION(ForeachExpr);

 public:
  static inline auto New(const BindingList bindings, Expression* body) -> ForeachExpr* {
    ASSERT(body);
    return new ForeachExpr(std::move(bindings), body);
  }

  static inline auto New(BindingExpr* binding, Expression* body) -> ForeachExpr* {
    ASSERT(body);
    return New(BindingList{binding}, body);
  }
};
}  // namespace gel::expr

#endif  // GEL_FOREACH_EXPR_H
