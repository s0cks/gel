#ifndef GEL_INVOKE_MACRO_EXPR_H
#define GEL_INVOKE_MACRO_EXPR_H

#include "gel/frontend/expr/expr.h"

namespace gel::expr {
class MacroExpansionSite {
  friend class InvokeMacroExpr;
  DEFINE_DEFAULT_COPYABLE_TYPE(MacroExpansionSite);

 private:
  Macro* target_;
  ExpressionList args_;

  inline void SetArgAt(const uint64_t idx, Expression* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfArgs());
    ASSERT(expr);
    args_[idx] = expr;
  }

 public:
  MacroExpansionSite(Macro* target, const ExpressionList& args) :
    target_(target),
    args_(args) {
    ASSERT(target_);
  }
  ~MacroExpansionSite() = default;

  auto GetTarget() const -> Macro* {
    return target_;
  }

  auto GetArgs() const -> const ExpressionList& {
    return args_;
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return args_.size();
  }

  auto GetArgAt(const uint64_t idx) const -> Expression* {
    ASSERT(idx >= 0 && idx <= GetNumberOfArgs());
    return args_[idx];
  }

  auto begin() const -> ExpressionList::const_iterator {
    return std::begin(args_);
  }

  auto end() const -> ExpressionList::const_iterator {
    return std::end(args_);
  }
};

class InvokeMacroExpr : public Expression {
  friend class gel::MacroEffectVisitor;

 private:
  MacroExpansionSite site_;

 protected:
  explicit InvokeMacroExpr(Macro* target, const ExpressionList& args) :  // NOLINT(modernize-pass-by-value)
    Expression(),
    site_(target, args) {}

  void SetArgAt(const uint64_t idx, Expression* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfArgs());
    ASSERT(expr);
    return site_.SetArgAt(idx, expr);
  }

 public:
  ~InvokeMacroExpr() override = default;

  auto GetExpansionSite() const -> const MacroExpansionSite& {
    return site_;
  }

  auto GetTarget() const -> Macro* {
    return site_.GetTarget();
  }

  inline auto HasTarget() const -> bool {
    return GetTarget() != nullptr;
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return site_.GetNumberOfArgs();
  }

  auto GetArgs() const -> const ExpressionList& {
    return site_.GetArgs();
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return GetNumberOfArgs();
  }

  auto GetArgAt(const uint64_t idx) const -> Expression* {
    return GetChildAt(idx);
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return site_.GetArgAt(idx);
  }

  auto VisitChildren(ExpressionVisitor& vis) -> bool override {
    for (const auto& arg : site_) {
      if (!arg->Accept(vis))
        return false;
    }
    return true;
  }

  DECLARE_EXPRESSION(InvokeMacroExpr);

 public:
  static inline auto New(Macro* target, const ExpressionList& args = {}) -> InvokeMacroExpr* {
    return new InvokeMacroExpr(target, args);
  }
};
}  // namespace gel::expr

#endif  // GEL_INVOKE_MACRO_EXPR_H
