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
  explicit Macro(Symbol* symbol, const ArgumentSet& args, expr::Expression* body) :
    symbol_(symbol),
    args_(args),
    body_(body) {
    ASSERT(symbol);
  }

 public:
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

  auto GetType() const -> Class* override {
    return GetClass();
  }

  DECLARE_TYPE(Macro);

 private:
  static Class* kClass;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

 public:
  static void Init();
  static inline auto New(Symbol* symbol, const ArgumentSet& args = {}, expr::Expression* body = nullptr) -> Macro* {
    return new Macro(symbol, args, body);
  }

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
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
