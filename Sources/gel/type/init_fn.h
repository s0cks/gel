#ifndef GEL_CONSTRUCTOR_H
#define GEL_CONSTRUCTOR_H

#include <string>

#include "gel/common.h"
#include "gel/compiled_code.h"
#include "gel/expr/expression.h"
#include "gel/expr/seq_expr.h"
#include "gel/local_scope.h"
#include "gel/object.h"
#include "gel/procedure.h"
#include "gel/type.h"

namespace gel {
class InitFn : public Fn {
  friend class Module;
  friend class Namespace;
  friend class MacroExpander;
  friend class FlowGraphCompiler;

 private:
  LocalScope* scope_ = nullptr;
  expr::SeqExpr* body_ = nullptr;
  CompiledCode* code_ = nullptr;

  explicit InitFn(Symbol* symbol, expr::SeqExpr* body) :
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
  ~InitFn() override = default;

  auto GetTargetName() const -> std::string {
    return HasSymbol() ? GetSymbol()->GetSymbolName() : "InitFn";
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

  friend auto operator<<(std::ostream& stream, const InitFn& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

  DECLARE_TYPE(InitFn);

 public:
  static inline auto New(Symbol* symbol, expr::SeqExpr* body = nullptr) -> InitFn* {
    ASSERT(symbol);
    return new InitFn(symbol, body);
  }
};

template <class T>
concept WithInit = requires(T value) {
  { value.GetInit() } -> std::same_as<InitFn*>;
  { value.HasInit() } -> std::same_as<bool>;
  { value.Init((Runtime*)nullptr) } -> std::same_as<bool>;
};
}  // namespace gel

#endif  // GEL_CONSTRUCTOR_H
