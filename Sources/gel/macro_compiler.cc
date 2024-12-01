#include "gel/macro_compiler.h"

#include "gel/expression.h"
#include "gel/macro.h"

namespace gel {
auto MacroCompiler::CompileMacro(expr::MacroDef* expr) -> Macro* {
  ASSERT(expr);
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  LocalVariable* local = nullptr;
  if (GetScope()->Lookup(symbol, &local, false))
    throw Exception(fmt::format("cannot redefine Symbol `{}`", symbol->Get()));
  return Macro::New(expr->GetSymbol(), expr->GetArgs(), expr->GetBody());
}
}  // namespace gel