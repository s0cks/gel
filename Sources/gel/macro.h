#ifndef GEL_MACRO_H
#define GEL_MACRO_H

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/expression.h"
#include "gel/object.h"

namespace gel {
class Parser;
class Macro : public Object {
  friend class Script;
  friend class Parser;

 private:
  Object* owner_ = nullptr;
  Symbol* symbol_ = nullptr;
  String* docstring_ = nullptr;
  LocalScope* scope_ = nullptr;
  ArgumentSet args_{};
  expr::ExpressionList body_{};

 protected:
  Macro() = default;
  Macro(Symbol* symbol, const ArgumentSet& args, const expr::ExpressionList& body) :  // NOLINT(modernize-pass-by-value)
    symbol_(symbol),
    args_(args),
    body_(body) {
    ASSERT(symbol);
  }

  void SetSymbol(Symbol* rhs) {
    ASSERT(rhs);
    symbol_ = rhs;
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

  void SetScope(LocalScope* rhs) {
    ASSERT(rhs);
    scope_ = rhs;
  }

  void SetArgs(const ArgumentSet& rhs) {
    args_ = rhs;
  }

  void SetBody(const expr::ExpressionList& rhs) {
    body_ = rhs;
  }

  void SetDocstring(String* rhs) {
    ASSERT(rhs);
    docstring_ = rhs;
  }

 public:
  ~Macro() override = default;

  auto GetOwner() const -> Object* {
    return owner_;
  }

  inline auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetDocstring() const -> String* {
    return docstring_;
  }

  inline auto HasDocstring() const -> bool {
    return GetDocstring() != nullptr;
  }

  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return args_.size();
  }

  inline auto HasArgs() const -> bool {
    return !args_.empty();
  }

  auto GetBody() const -> const expr::ExpressionList& {
    return body_;
  }

  inline auto IsEmpty() const -> bool {
    return body_.empty();
  }

  DECLARE_TYPE(Macro);

 private:
  static inline auto New() -> Macro* {
    return new Macro();
  }

 public:
  static inline auto New(Symbol* symbol, const ArgumentSet& args = {}, const expr::ExpressionList& body = {}) -> Macro* {
    return new Macro(symbol, args, body);
  }
};
}  // namespace gel

#endif  // GEL_MACRO_H
