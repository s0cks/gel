#ifndef SCM_MACRO_H
#define SCM_MACRO_H

#include "scheme/type.h"

namespace scm {
class Macro : public Type {
 private:
  Symbol* symbol_;

 protected:
  explicit Macro(Symbol* symbol) :
    symbol_(symbol) {
    ASSERT(symbol);
  }

 public:
  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  DECLARE_TYPE(Macro);

 public:
  static inline auto New(Symbol* symbol) -> Macro* {
    return new Macro(symbol);
  }
};
}  // namespace scm

#endif  // SCM_MACRO_H
