#ifndef GEL_SCRIPT_H
#define GEL_SCRIPT_H

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/lambda.h"
#include "gel/local_scope.h"
#include "gel/namespace.h"

namespace gel {
class Script : public Object, public Executable {
  friend class Parser;
  friend class MacroExpander;
  friend class ScriptCompiler;
  friend class FlowGraphCompiler;
  using LambdaList = std::vector<Lambda*>;
  using MacroList = std::vector<Macro*>;

 private:
  LocalScope* scope_;
  String* name_ = nullptr;
  MacroList macros_{};
  LambdaList lambdas_{};
  NamespaceList namespaces_{};
  expr::ExpressionList body_{};

 protected:
  explicit Script(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
  }

  void SetName(String* name) {
    ASSERT(name);
    name_ = name;
  }

  inline auto at(const uint64_t idx) const -> expr::ExpressionList::const_iterator {
    return std::begin(body_) + static_cast<expr::ExpressionList::difference_type>(idx);
  }

  inline void Append(expr::Expression* expr) {
    ASSERT(expr);
    body_.push_back(expr);
  }

  inline void InsertAt(const uint64_t idx, expr::Expression* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfExpressions());
    ASSERT(expr);
    body_.insert(at(idx), expr);
  }

  inline void InsertAt(const uint64_t idx, const expr::ExpressionList& exprs) {
    ASSERT(idx >= 0 && idx <= GetNumberOfExpressions());
    ASSERT(!exprs.empty());
    body_.insert(at(idx), std::begin(exprs), std::end(exprs));
  }

  void Append(Macro* macro);
  void Append(Lambda* lambda);
  void Append(Namespace* ns);

  auto VisitPointers(PointerVisitor* vis) -> bool override;

  void SetExpressionAt(const uint64_t idx, expr::Expression* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfExpressions());
    ASSERT(expr);
    body_[idx] = expr;
  }

  void RemoveExpressionAt(const uint64_t idx) {
    ASSERT(idx >= 0 && idx <= GetNumberOfExpressions());
    body_.erase(at(idx));
  }

  void ReplaceExpressionAt(const uint64_t idx, expr::Expression* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfExpressions());
    ASSERT(expr);
    RemoveExpressionAt(idx);
    InsertAt(idx, expr);
  }

  void ReplaceExpressionAt(const uint64_t idx, const expr::ExpressionList& body) {
    ASSERT(idx >= 0 && idx <= GetNumberOfExpressions());
    ASSERT(!body.empty());
    RemoveExpressionAt(idx);
    InsertAt(idx, body);
  }

 public:
  ~Script() override = default;

  auto GetName() const -> String* {
    return name_;
  }

  auto HasName() const -> bool {
    return GetName() != nullptr;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto HasScope() const -> bool {
    return GetScope() != nullptr;
  }

  auto GetBody() const -> const expr::ExpressionList& {
    return body_;
  }

  inline auto IsEmpty() const -> bool {
    return body_.empty();
  }

  auto GetNumberOfExpressions() const -> uint64_t {
    return body_.size();
  }

  auto GetExpressionAt(const uint64_t idx) const -> expr::Expression* {
    ASSERT(idx >= 0 && idx <= GetNumberOfExpressions());
    return body_[idx];
  }

  auto GetFullyQualifiedName() const -> std::string {
    return HasName() ? GetName()->Get() : "Script";
  }

  inline auto HasExpressionAt(const uint64_t idx) const -> bool {
    return GetExpressionAt(idx) != nullptr;
  }

  DECLARE_TYPE(Script);

 public:
  static inline auto New(LocalScope* scope) -> Script* {
    ASSERT(scope);
    return new Script(scope);
  }

  static auto FromFile(const std::string& filename, const bool compile = true) -> Script*;
};
}  // namespace gel

#endif  // GEL_SCRIPT_H
