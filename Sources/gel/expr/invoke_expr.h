#ifndef GEL_INVOKE_EXPR_H
#define GEL_INVOKE_EXPR_H

#include "gel/expr/expression.h"

namespace gel::expr {
template <class Target>
class TemplateInvokeExpr : public Expression {
  DEFINE_NON_COPYABLE_TYPE(TemplateInvokeExpr);

 private:
  Target* target_;
  ExpressionList args_{};

 protected:
  TemplateInvokeExpr(Target* target, const ExpressionList& args) :
    Expression(),
    target_(target),
    args_(args) {
    ASSERT(target_);
  }

  void SetTarget(Target* target) {
    ASSERT(target);
    target_ = target;
  }

  void SetArgAt(const uint64_t idx, Expression* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfArgs());
    ASSERT(expr);
    args_[idx] = expr;
  }

  void AddArgs(const ExpressionList& args) {
    args_.insert(std::end(args_), std::begin(args), std::end(args));
  }

 public:
  ~TemplateInvokeExpr() override = default;

  auto GetTarget() const -> Target* {
    return target_;
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return args_.size();
  }

  auto GetArgs() const -> const ExpressionList& {
    return args_;
  }

  virtual auto GetArgAt(const uint64_t idx) const -> Expression* {
    ASSERT(idx >= 0 && idx <= GetNumberOfArgs());
    return args_[idx];
  }

  inline auto HasArgs() const -> bool {
    return GetNumberOfArgs() > 0;
  }

  inline auto HasArgAt(const uint64_t idx) const -> bool {
    ASSERT(idx >= 0 && idx <= GetNumberOfArgs());
    return GetArgAt(idx) != nullptr;
  }

  auto VisitArgs(ExpressionVisitor& vis) -> bool {
    for (const auto& arg : args_) {
      ASSERT(arg);
      if (!arg->Accept(vis))
        return false;
    }
    return true;
  }
};

class InvokeExpr : public TemplateInvokeExpr<Expression> {
  friend class gel::MacroEffectVisitor;

 protected:
  explicit InvokeExpr(Expression* target, const ExpressionList& args) :  // NOLINT(modernize-pass-by-value)
    TemplateInvokeExpr(target, args) {}

 public:
  ~InvokeExpr() override = default;

  auto GetNumberOfChildren() const -> uint64_t override {
    return GetNumberOfArgs() + 1;
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return idx == 0 ? GetTarget() : GetArgAt(idx - 1);
  }

  auto VisitChildren(ExpressionVisitor& vis) -> bool override {
    if (!GetTarget()->Accept(vis))
      return false;
    return VisitArgs(vis);
  }

  auto VisitTarget(ExpressionVisitor& vis) -> bool {
    return GetTarget()->Accept(vis);
  }

  auto IsMacroCall(LocalScope* scope) const -> bool;
  DECLARE_EXPRESSION(InvokeExpr);

 public:
  static inline auto New(Expression* target, const ExpressionList& args = {}) -> InvokeExpr* {
    return new InvokeExpr(target, args);
  }
};
}  // namespace gel::expr

#endif  // GEL_INVOKE_EXPR_H
