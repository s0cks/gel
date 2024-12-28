#ifndef GEL_MACRO_EXPANDER_H
#define GEL_MACRO_EXPANDER_H

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/local.h"
#include "gel/local_scope.h"

namespace gel {
class Macro;
class MacroExpander {
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

#define DECLARE_VISIT(Name) auto Visit##Name(expr::Name* expr)->bool override;
  FOR_EACH_EXPRESSION_NODE(DECLARE_VISIT)
#undef DECLARE_VISIT
};
}  // namespace gel

#endif  // GEL_MACRO_EXPANDER_H
