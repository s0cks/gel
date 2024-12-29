#ifndef GEL_LAMBDA_H
#define GEL_LAMBDA_H

#include <fmt/base.h>

#include <set>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/expression.h"
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

class Lambda : public Procedure, public Executable {
  friend class Parser;
  friend class Module;
  friend class MacroExpander;
  friend class FlowGraphCompiler;

 private:
  Object* owner_ = nullptr;
  String* docstring_ = nullptr;
  LocalScope* scope_ = nullptr;
  ArgumentSet args_;           // TODO: fails to copy during GC
  expr::ExpressionList body_;  // TODO: fails to copy during GC

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

  void SetArgs(const ArgumentSet& args) {
    args_ = args;
  }

  void SetBody(const expr::ExpressionList& body) {
    body_ = body;
  }

  void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

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

 protected:
  Lambda(Symbol* symbol, ArgumentSet args, const expr::ExpressionList& body) :  // NOLINT(modernize-pass-by-value)
    Procedure(symbol),
    args_(args),
    body_(body) {}

  auto VisitPointers(PointerVisitor* vis) -> bool override;

 public:
  ~Lambda() override = default;

  auto GetOwner() const -> Object* {
    return owner_;
  }

  inline auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

  auto GetDocstring() const -> String* {
    return docstring_;
  }

  inline auto HasDocstring() const -> bool {
    return GetDocstring() != nullptr;
  }

  void SetDocstring(String* rhs) {
    ASSERT(rhs);
    docstring_ = rhs;
  }

  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetBody() const -> const expr::ExpressionList& {
    return body_;
  }

  auto GetNumberOfExpressions() const -> uint64_t {
    return body_.size();
  }

  inline auto IsEmpty() const -> bool {
    return body_.empty();
  }

  auto GetExpressionAt(const uint64_t idx) const -> expr::Expression* {
    ASSERT(idx >= 0 && idx <= GetNumberOfExpressions());
    return body_[idx];
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return args_.size();
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
  }

  DECLARE_TYPE(Lambda);

 public:
  static inline auto New(Symbol* name, const ArgumentSet& args, const expr::ExpressionList& body) -> Lambda* {
    return new Lambda(name, args, body);
  }

  static inline auto New(const ArgumentSet& args = {}, const expr::ExpressionList& body = {}) -> Lambda* {
    return new Lambda(nullptr, args, body);
  }
};
}  // namespace gel

#endif  // GEL_LAMBDA_H
