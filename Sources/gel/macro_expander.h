#ifndef GEL_MACRO_EXPANDER_H
#define GEL_MACRO_EXPANDER_H

#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/expr/expression_dot.h"
#include "gel/flags.h"
#include "gel/local.h"
#include "gel/local_scope.h"

namespace gel {
class Macro;
class MacroExpander {
  friend class ExpanderScope;
  friend class MacroEffectVisitor;

  class ExpanderScope {
    DEFINE_NON_COPYABLE_TYPE(ExpanderScope);

   private:
    MacroExpander* owner_;

   public:
    ExpanderScope(MacroExpander* owner) :
      owner_(owner) {
      ASSERT(owner_);
      GetOwner()->PushScope();
    }
    ~ExpanderScope() {
      GetOwner()->PopScope();
    }

    auto GetOwner() const -> MacroExpander* {
      return owner_;
    }

    auto operator->() const -> LocalScope* {
      return GetOwner()->GetScope();
    }

    operator LocalScope*() const {
      return GetOwner()->GetScope();
    }
  };

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

  template <class T>
  auto ExpandAll(T* target, std::enable_if_t<gel::has_code<T>::value>* = nullptr) -> bool;

 public:
  template <class T>
  static inline void ExpandAll(T* target, LocalScope* locals, std::enable_if_t<gel::has_code<T>::value>* = nullptr) {
    ASSERT(target);
    ASSERT(locals);
    const auto target_name = target->GetTargetName();
    if (FLAGS_dump_ast) {
      expr::GenerateExprDotPng(fmt::format("reports/{}-pre-expansion.png", target_name), target_name, target->GetBody());
    }
    MacroExpander expander(locals);
    LOG_IF(FATAL, !expander.ExpandAll(target)) << "failed to expand macros in " << target->ToString();
    if (FLAGS_dump_ast) {
      if (FLAGS_dump_ast) {
        expr::GenerateExprDotPng(fmt::format("reports/{}-post-expansion.png", target_name), target_name, target->GetBody());
      }
    }
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

template <class T>
auto MacroExpander::ExpandAll(T* target, std::enable_if_t<gel::has_code<T>::value>*) -> bool {
  ASSERT(target);
  ExpanderScope scope(this);
  if (target->HasScope())
    scope->AddAll(target->GetScope());
  MacroEffectVisitor for_effect(this);
  if (!target->GetBody()->Accept(&for_effect)) {
    LOG(ERROR) << "failed to visit " << target->ToString() << " body.";
    return false;
  }
  return true;
}
}  // namespace gel

#endif  // GEL_MACRO_EXPANDER_H
