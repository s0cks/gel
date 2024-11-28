#ifndef SCM_MACRO_H
#define SCM_MACRO_H

#include "scheme/argument.h"
#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/object.h"

namespace scm {
class Macro : public Object {
 private:
  Symbol* symbol_;
  ArgumentSet args_;
  expr::Expression* body_;

 protected:
  explicit Macro(Symbol* symbol, const ArgumentSet& args, expr::Expression* body) :  // NOLINT(modernize-pass-by-value)
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

  auto GetBody() const -> expr::Expression* {
    return body_;
  }

  inline auto HasBody() const -> bool {
    return GetBody() != nullptr;
  }

  DECLARE_TYPE(Macro);

 public:
  static inline auto New(Symbol* symbol, const ArgumentSet& args = {}, expr::Expression* body = nullptr) -> Macro* {
    return new Macro(symbol, args, body);
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
