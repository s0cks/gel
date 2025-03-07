#ifndef GEL_LAMBDA_H
#define GEL_LAMBDA_H

#include <fmt/base.h>

#include <set>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/compiled_code.h"
#include "gel/expr/expression.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/procedure.h"

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
  CompiledCode code_{};

  void SetBody(expr::SeqExpr* body) {
    body_ = body;
  }

  void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

  void SetCode(const CompiledCode& rhs) {
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

  auto GetCode() const -> const CompiledCode& {
    return code_;
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
}  // namespace gel

#endif  // GEL_LAMBDA_H
