#ifndef GEL_LAMBDA_H
#define GEL_LAMBDA_H

#include <fmt/base.h>
#include <string>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/compiled_code.h"
#include "gel/expr/expression.h"
#include "gel/expr/seq_expr.h"
#include "gel/local_scope.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/procedure.h"
#include "gel/type.h"

namespace gel {
class Parser;
class MacroExpander;
namespace expr {
class LambdaFnExpr;
class Expression;
}  // namespace expr
namespace ir {
class GraphEntryInstr;
}  // namespace ir

class LambdaFn : public Fn {
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
  LambdaFn(Symbol* symbol, Array<Argument*>* args, expr::SeqExpr* body = nullptr) :
    Fn(symbol),
    body_(body) {
    if (args)
      SetArgs(args);
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~LambdaFn() override = default;

  auto GetTargetName() const -> std::string {
    return HasSymbol() ? GetSymbol()->GetSymbolName() : "LambdaFnFn";
  }

  auto GetScope() const -> LocalScope* {  // TODO: this should never return nullptr
    return scope_;
  }

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
  }

  auto GetFullyQualifiedName() const -> std::string {
    return HasSymbol() ? GetSymbol()->GetFullyQualifiedName() : "LambdaFnFn";
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

  friend auto operator<<(std::ostream& stream, const LambdaFn& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

  DECLARE_TYPE(LambdaFn);

 public:
  static inline auto New(Symbol* name, Array<Argument*>* args, expr::SeqExpr* body = nullptr) -> LambdaFn* {
    return new LambdaFn(name, args, body);
  }

  static inline auto New(Array<Argument*>* args = nullptr, expr::SeqExpr* body = nullptr) -> LambdaFn* {
    return new LambdaFn(nullptr, args, body);
  }
};
static_assert(WithSymbol<LambdaFn>);
static_assert(HasMutableSymbol<LambdaFn>);
static_assert(HasDocstring<LambdaFn>);
}  // namespace gel

#endif  // GEL_LAMBDA_H
