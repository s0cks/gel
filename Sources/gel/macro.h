#ifndef GEL_MACRO_H
#define GEL_MACRO_H

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/expression.h"
#include "gel/object.h"

namespace gel {
class Macro : public Object {
 private:
  Symbol* symbol_;
  ArgumentSet args_;
  expr::ExpressionList body_{};

 protected:
  explicit Macro(Symbol* symbol, const ArgumentSet& args, const expr::ExpressionList& body) :  // NOLINT(modernize-pass-by-value)
    symbol_(symbol),
    args_(args),
    body_(body) {
    ASSERT(symbol);
  }

 public:
  ~Macro() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetBody() const -> const expr::ExpressionList& {
    return body_;
  }

  inline auto IsEmpty() const -> bool {
    return body_.empty();
  }

  DECLARE_TYPE(Macro);

 public:
  static inline auto New(Symbol* symbol, const ArgumentSet& args = {}, const expr::ExpressionList& body = {}) -> Macro* {
    return new Macro(symbol, args, body);
  }
};
}  // namespace gel

#endif  // GEL_MACRO_H
