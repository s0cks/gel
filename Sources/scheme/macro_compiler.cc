#include "scheme/macro_compiler.h"

#include "scheme/expression.h"
#include "scheme/macro.h"

namespace scm {
auto MacroCompiler::CompileMacro(expr::MacroDef* expr) -> Macro* {
  ASSERT(expr);
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  LocalVariable* local = nullptr;
  if (GetScope()->Lookup(symbol, &local, false))
    throw Exception(fmt::format("cannot redefine Symbol `{}`", symbol->Get()));
  return Macro::New(expr->GetSymbol(), expr->GetArgs(), expr->GetBody());
}
}  // namespace scm