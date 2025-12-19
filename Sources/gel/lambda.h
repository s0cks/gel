#ifndef GEL_LAMBDA_H
#define GEL_LAMBDA_H

#include <fmt/base.h>
#include <string>

#include "argument.h"
#include "common.h"
#include "compiled_code.h"
#include "expr/expression.h"
#include "expr/seq_expr.h"
#include "local_scope.h"
#include "native_procedure.h"
#include "object.h"
#include "pointer.h"
#include "procedure.h"
#include "type.h"

namespace gel {
class Parser;
class MacroExpander;
namespace expr {
class LambdaExpr;
class Expression;
}  // namespace expr
namespace ir {
class GraphEntryInstr;
}  // namespace ir

class Lambda : public Procedure {
  friend class Parser;
  friend class Module;
  friend class Runtime;
  friend class Namespace;
  friend class MacroExpander;
  friend class FlowGraphCompiler;

 private:
  LocalScope* scope_ = nullptr;
  expr::SeqExpr* body_ = nullptr;
  CompiledCode* code_ = nullptr;

  void SetBody(expr::SeqExpr* body) {
    body_ = body;
  }

  void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

  void SetCode(CompiledCode* rhs) {
    ASSERT(rhs);
    code_ = rhs;
  }

 protected:
  // TODO: remove args from constructor
  Lambda(Symbol* symbol, Array<Argument*>* args, expr::SeqExpr* body = nullptr) :
    Procedure(symbol),
    body_(body) {
    if (args)
      SetArgs(args);
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~Lambda() override = default;

  auto GetTargetName() const -> std::string {
    return HasSymbol() ? GetSymbol()->GetSymbolName() : "Lambda";
  }

  auto GetScope() const -> LocalScope* {  // TODO: this should never return nullptr
    return scope_;
  }

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
  }

  auto GetFullyQualifiedName() const -> std::string {
    return HasSymbol() ? GetSymbol()->GetFullyQualifiedName() : "Lambda";
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

  inline auto IsCompiled() const -> bool {
    return GetCode() != nullptr;
  }

  friend auto operator<<(std::ostream& stream, const Lambda& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

  DECLARE_TYPE(Lambda);

 public:
  static inline auto New(Symbol* name, Array<Argument*>* args, expr::SeqExpr* body = nullptr) -> Lambda* {
    return new Lambda(name, args, body);
  }

  static inline auto New(Array<Argument*>* args = nullptr, expr::SeqExpr* body = nullptr) -> Lambda* {
    return new Lambda(nullptr, args, body);
  }
};
static_assert(WithSymbol<Lambda>);
static_assert(HasMutableSymbol<Lambda>);
static_assert(HasDocstring<Lambda>);
}  // namespace gel

#endif  // GEL_LAMBDA_H
