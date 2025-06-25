#ifndef GEL_SCRIPT_H
#define GEL_SCRIPT_H

#include "gel/common.h"
#include "gel/compiled_code.h"
#include "gel/expression.h"
#include "gel/lambda.h"
#include "gel/local_scope.h"
#include "gel/namespace.h"

namespace gel {
class Script : public Object {
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
  expr::SeqExpr* body_ = nullptr;
  CompiledCode* code_ = nullptr;

 protected:
  explicit Script(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
  }

  void SetName(String* name) {
    ASSERT(name);
    name_ = name;
  }

  inline void Append(const MacroList& macros) {
    macros_.insert(std::end(macros_), std::begin(macros), std::end(macros));
  }

  inline void Append(const NamespaceList& namespaces) {
    namespaces_.insert(std::end(namespaces_), std::begin(namespaces), std::end(namespaces));
  }

  void Append(Macro* macro);
  void Append(Lambda* lambda);
  void Append(Namespace* ns);

  auto VisitPointers(PointerVisitor* vis) -> bool override;

  void AddChild(Object* rhs) override {
    ASSERT(rhs);
    if (rhs->IsMacro()) {
      macros_.push_back(rhs->AsMacro());
    } else if (rhs->IsLambda()) {
      lambdas_.push_back(rhs->AsLambda());
    } else if (rhs->IsNamespace()) {
      namespaces_.push_back(rhs->AsNamespace());
    }
  }

  void SetBody(expr::SeqExpr* rhs) {
    ASSERT(rhs);
    body_ = rhs;
  }

  void SetCode(CompiledCode* rhs) {
    ASSERT(rhs);
    code_ = rhs;
  }

 public:
  ~Script() override = default;

  auto GetTargetName() const -> std::string {
    return HasName() ? GetName()->Get() : "Script";
  }

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

  auto GetFullyQualifiedName() const -> std::string {
    return HasName() ? GetName()->Get() : "Script";
  }

  auto GetBody() const -> expr::SeqExpr* {
    return body_;
  }

  inline auto HasBody() const -> bool {
    return GetBody() != nullptr;
  }

  inline auto IsEmpty() const -> bool {
    return !HasBody() || GetBody()->IsEmpty();
  }

  auto GetCode() const -> CompiledCode* {
    return code_;
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
