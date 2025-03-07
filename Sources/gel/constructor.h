#ifndef GEL_CONSTRUCTOR_H
#define GEL_CONSTRUCTOR_H

#include "gel/expr/expression.h"
#include "gel/procedure.h"

namespace gel {
class Constructor : public Procedure, public Executable {
  friend class Module;
  friend class Namespace;
  friend class MacroExpander;

 private:
  LocalScope* scope_ = nullptr;
  expr::ExpressionList body_{};

  explicit Constructor(Symbol* symbol, const expr::ExpressionList& body) :
    Procedure(symbol),
    Executable(),
    body_(body) {}

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

  void SetScope(LocalScope* rhs) {
    ASSERT(rhs);
    scope_ = rhs;
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

 public:
  ~Constructor() override = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetNumberOfExpressions() const -> uint64_t {
    return body_.size();
  }

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
  }

  auto IsEmpty() const -> bool {  // TODO: refactor
    return body_.empty();
  }

  auto GetBody() const -> const expr::ExpressionList& {
    return body_;
  }

  auto GetFullyQualifiedName() const -> std::string {
    return GetSymbol()->GetFullyQualifiedName();
  }

  auto GetExpressionAt(const uint64_t idx) const -> expr::Expression* {
    ASSERT(idx >= 0 && idx <= GetNumberOfExpressions());
    return body_[idx];
  }

  DECLARE_TYPE(Constructor);

 public:
  static inline auto New(Symbol* symbol, const expr::ExpressionList& body = {}) -> Constructor* {
    ASSERT(symbol);
    return new Constructor(symbol, body);
  }
};
}  // namespace gel

#endif  // GEL_CONSTRUCTOR_H
