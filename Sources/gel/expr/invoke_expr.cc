#include "gel/expr/invoke_expr.h"

#include "gel/expr/literal_expr.h"
#include "gel/to_string_helper.h"

namespace gel::expr {
auto InvokeExpr::IsMacroCall(LocalScope* scope) const -> bool {
  ASSERT(scope);
  if (!expr::IsLiteralSymbol(GetTarget()))
    return false;
  const auto symbol = GetTarget()->AsLiteralExpr()->GetValue()->AsSymbol();
  ASSERT(symbol);
  LocalVariable* local = nullptr;
  if (!scope->Lookup(symbol, &local))
    return false;
  ASSERT(local);
  return local->HasValue() && local->GetValue()->IsMacro();
}

auto InvokeExpr::ToString() const -> std::string {
  ToStringHelper<InvokeExpr> helper{};
  helper.AddField("target", GetTarget());
  helper.AddField("num_args", GetNumberOfArgs());
#ifdef GEL_DEBUG
  helper.AddField("args", GetArgs());
#endif  // GEL_DEBUG
  return helper;
}
}  // namespace gel::expr