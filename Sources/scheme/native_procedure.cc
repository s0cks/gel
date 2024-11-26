#include "scheme/native_procedure.h"

#include "scheme/runtime.h"

namespace scm {
auto NativeProcedure::CreateClass() -> Class* {
  return Class::New(Procedure::GetClass(), "NativeProcedure");
}

auto NativeProcedure::Return(Object* rhs) const -> bool {
  ASSERT(rhs);
  GetRuntime()->Push(rhs);
  return DoNothing();
}

auto NativeProcedure::Apply(const std::vector<Object*>& rhs) const -> bool {
  return ApplyProcedure(rhs);
}

auto NativeProcedure::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsNativeProcedure())
    return false;
  return GetSymbol()->Equals(rhs->AsNativeProcedure()->GetSymbol());
}

auto NativeProcedure::ToString() const -> std::string {
  std::stringstream ss;
  ss << "NativeProcedure(";
  ss << "symbol=" << GetSymbol();
  ss << ")";
  return ss.str();
}

}  // namespace scm