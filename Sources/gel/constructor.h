#ifndef GEL_CONSTRUCTOR_H
#define GEL_CONSTRUCTOR_H

#include <string>

#include "common.h"
#include "compiled_code.h"
#include "expr/expression.h"
#include "expr/seq_expr.h"
#include "local_scope.h"
#include "object.h"
#include "procedure.h"
#include "type.h"

namespace gel {
class Constructor : public Fn {
  friend class Module;
  friend class Namespace;
  friend class MacroExpander;
  friend class FlowGraphCompiler;

 private:
  LocalScope* scope_ = nullptr;
  expr::SeqExpr* body_ = nullptr;
  CompiledCode* code_ = nullptr;

  explicit Constructor(Symbol* symbol, expr::SeqExpr* body) :
    Fn(symbol),
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

  inline auto IsCompiled() const -> bool {
    return GetCode() != nullptr;
  }

  friend auto operator<<(std::ostream& stream, const Constructor& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

  DECLARE_TYPE(Constructor);

 public:
  static inline auto New(Symbol* symbol, expr::SeqExpr* body = nullptr) -> Constructor* {
    ASSERT(symbol);
    return new Constructor(symbol, body);
  }
};

template <class T>
concept WithInit = requires(T value) {
  { value.GetInit() } -> std::same_as<Constructor*>;
  { value.HasInit() } -> std::same_as<bool>;
  { value.Init((Runtime*)nullptr) } -> std::same_as<bool>;
};
}  // namespace gel

#endif  // GEL_CONSTRUCTOR_H
