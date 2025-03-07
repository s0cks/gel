#ifndef GEL_CONSTRUCTOR_H
#define GEL_CONSTRUCTOR_H

#include "gel/compiled_code.h"
#include "gel/expr/expression.h"
#include "gel/procedure.h"

namespace gel {
class Constructor : public Procedure {
  friend class Module;
  friend class Namespace;
  friend class MacroExpander;
  friend class FlowGraphCompiler;

 private:
  LocalScope* scope_ = nullptr;
  expr::SeqExpr* body_ = nullptr;
  CompiledCode code_{};

  explicit Constructor(Symbol* symbol, expr::SeqExpr* body) :
    Procedure(symbol),
    body_(body) {}

  void SetScope(LocalScope* rhs) {
    ASSERT(rhs);
    scope_ = rhs;
  }

  void SetCode(const CompiledCode& rhs) {
    code_ = rhs;
  }

 public:
  ~Constructor() override = default;

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

  auto GetCode() const -> const CompiledCode& {
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
