#ifndef GEL_CONSTRUCTOR_H
#define GEL_CONSTRUCTOR_H

#include <string>

#include "gel/common.h"
#include "gel/compiled_code.h"
#include "gel/expression.h"
#include "gel/local_scope.h"
#include "gel/object.h"
#include "gel/procedure.h"
#include "gel/type.h"

namespace gel {
class Constructor : public Procedure {
  friend class Module;
  friend class Namespace;
  friend class MacroExpander;
  friend class FlowGraphCompiler;

 private:
  LocalScope* scope_ = nullptr;
  expr::SeqExpr* body_ = nullptr;
  CompiledCode* code_ = nullptr;

  explicit Constructor(Symbol* symbol, expr::SeqExpr* body) :
    Procedure(symbol),
    body_(body) {}

  void SetScope(LocalScope* rhs) {
    ASSERT(rhs);
    scope_ = rhs;
  }

  void SetCode(CompiledCode* rhs) {
    ASSERT(rhs);
    code_ = rhs;
  }

 public:
  ~Constructor() override = default;

  auto GetTargetName() const -> std::string {
    return HasSymbol() ? GetSymbol()->GetSymbolName() : "Constructor";
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
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

  auto GetFullyQualifiedName() const -> std::string {
    return GetSymbol()->GetFullyQualifiedName();
  }

  auto GetCode() const -> CompiledCode* {
    return code_;
  }

  DECLARE_TYPE(Constructor);

 public:
  static inline auto New(Symbol* symbol, expr::SeqExpr* body = nullptr) -> Constructor* {
    ASSERT(symbol);
    return new Constructor(symbol, body);
  }
};
}  // namespace gel

#endif  // GEL_CONSTRUCTOR_H
