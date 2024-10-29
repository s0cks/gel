#ifndef SCM_MACRO_H
#define SCM_MACRO_H

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/type.h"

namespace scm {
class Macro : public Type {
 private:
  Symbol* symbol_;
  expr::Expression* body_;

 protected:
  explicit Macro(Symbol* symbol, expr::Expression* body) :
    symbol_(symbol),
    body_(body) {
    ASSERT(symbol);
  }

 public:
  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetBody() const -> expr::Expression* {
    return body_;
  }

  inline auto HasBody() const -> bool {
    return GetBody() != nullptr;
  }

  DECLARE_TYPE(Macro);

 public:
  static inline auto New(Symbol* symbol, expr::Expression* body = nullptr) -> Macro* {
    return new Macro(symbol, body);
  }
};

class MacroExpander {
  DEFINE_NON_COPYABLE_TYPE(MacroExpander);

 public:
  MacroExpander() = default;
  virtual ~MacroExpander() = default;
  auto Expand(expr::Expression** expr) -> bool;
};
}  // namespace scm

#endif  // SCM_MACRO_H
