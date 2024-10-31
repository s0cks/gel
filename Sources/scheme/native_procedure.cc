#include "scheme/native_procedure.h"

#include "scheme/runtime.h"

namespace scm {
auto NativeProcedure::ToString() const -> std::string {
  std::stringstream ss;
  ss << "NativeProcedure(";
  ss << "symbol=" << GetSymbol();
  ss << ")";
  return ss.str();
}

auto NativeProcedure::Apply(Runtime* runtime) const -> bool {
  ASSERT(runtime);
  runtime->PushError("Invalid");
  return true;
}
}  // namespace scm