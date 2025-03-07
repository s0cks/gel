#ifndef GEL_MACRO_EXPANDER_H
#define GEL_MACRO_EXPANDER_H

#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/local.h"
#include "gel/local_scope.h"

namespace gel {
class Macro;
class MacroExpander {
  friend class ExpanderScope;
  friend class MacroEffectVisitor;
  DEFINE_NON_COPYABLE_TYPE(MacroExpander);

 private:
  LocalScope* scope_;

  inline auto PushScope(const std::vector<LocalScope*>& scopes = {}) -> LocalScope* {
    const auto new_scope = LocalScope::Union(scopes, GetScope());
    ASSERT(new_scope);
    scope_ = new_scope;
    return new_scope;
  }

  inline void PopScope() {
    ASSERT(scope_ && scope_->HasParent());
    scope_ = scope_->GetParent();
  }

 public:
  explicit MacroExpander(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
  }
  ~MacroExpander() = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto ExpandAllInLambda(Lambda* lambda) -> bool;
  auto ExpandAllInConstructor(Constructor* lambda) -> bool;
  auto ExpandAllInScript(Script* script) -> bool;

 public:
  static inline void ExpandAll(Script* script, LocalScope* scope) {
    ASSERT(script);
    ASSERT(scope);
    MacroExpander expander(scope);
    LOG_IF(FATAL, !expander.ExpandAllInScript(script)) << "failed to expand macros in " << script;
  }

  static inline void ExpandAll(Lambda* lambda, LocalScope* scope) {
    ASSERT(lambda);
    ASSERT(scope);
    MacroExpander expander(scope);
    LOG_IF(FATAL, !expander.ExpandAllInLambda(lambda)) << "failed to expand macros in " << lambda;
  }

  static inline void ExpandAll(Constructor* init, LocalScope* scope) {
    ASSERT(init);
    ASSERT(scope);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    MacroExpander expander(scope);
    LOG_IF(FATAL, !expander.ExpandAllInConstructor(init)) << "failed to expand macros in " << init;
  }
};

class MacroEffectVisitor : public ExpressionVisitor {
  DEFINE_NON_COPYABLE_TYPE(MacroEffectVisitor);

 private:
  MacroExpander* owner_;
  expr::ExpressionList result_{};

 protected:
  void SetResult(const expr::ExpressionList& result) {
    result_ = result;
  }

  inline void SetResult(expr::Expression* expr) {
    ASSERT(expr);
    return SetResult(expr::ExpressionList{expr});
  }

  inline void SetResult(MacroEffectVisitor& vis) {
    return SetResult(vis.GetResults());
  }

  virtual auto Expand(expr::LiteralExpr* expr, expr::ExpressionList& result) -> bool {
    return false;
  }

  virtual auto VisitExpressionList(const expr::ExpressionList& source, expr::ExpressionList& dest, bool* changed) -> bool;

 public:
  explicit MacroEffectVisitor(MacroExpander* owner) :
    ExpressionVisitor(),
    owner_(owner) {
    ASSERT(owner_);
  }
  ~MacroEffectVisitor() override = default;

  auto GetOwner() const -> MacroExpander* {
    return owner_;
  }

  auto GetResults() const -> const expr::ExpressionList& {
    return result_;
  }

  inline auto GetNumberOfResults() const -> uword {
    return result_.size();
  }

  auto GetResult() const -> expr::Expression* {
    ASSERT(HasResult());
    return result_[0];
  }

  auto begin() const -> expr::ExpressionList::const_iterator {
    return std::begin(GetResults());
  }

  auto end() const -> expr::ExpressionList::const_iterator {
    return std::end(GetResults());
  }

  inline auto HasResult() const -> bool {
    return !result_.empty();
  }

  operator bool() const {
    return HasResult();
  }

  auto operator()(expr::Expression* expr) -> bool {
    ASSERT(expr);
    return expr->Accept(this);
  }

#define DECLARE_VISIT(Name) auto Visit##Name(expr::Name* expr)->bool override;
  FOR_EACH_EXPRESSION_NODE(DECLARE_VISIT)
#undef DECLARE_VISIT
};

class MacroExpansionSiteEffectVisitor : public MacroEffectVisitor {
  DEFINE_NON_COPYABLE_TYPE(MacroExpansionSiteEffectVisitor);

 private:
  expr::MacroExpansionSite site_;

 protected:
  auto Expand(expr::LiteralExpr* expr, expr::ExpressionList& results) -> bool override;
  auto VisitExpressionList(const expr::ExpressionList& source, expr::ExpressionList& dest, bool* changed) -> bool override;

 public:
  MacroExpansionSiteEffectVisitor(MacroExpander* owner, const expr::MacroExpansionSite& site) :
    MacroEffectVisitor(owner),
    site_(site) {}
  ~MacroExpansionSiteEffectVisitor() override = default;

  auto GetSite() const -> const expr::MacroExpansionSite& {
    return site_;
  }

  auto VisitInvokeExpr(expr::InvokeExpr* expr) -> bool override;
  auto VisitWhenExpr(expr::WhenExpr* expr) -> bool override;
  auto VisitLiteralExpr(expr::LiteralExpr* expr) -> bool override;
};
}  // namespace gel

#endif  // GEL_MACRO_EXPANDER_H
