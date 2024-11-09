#include "scheme/native_procedure.h"

#include "scheme/runtime.h"

namespace scm {
Class* NativeProcedure::kClass = nullptr;
void NativeProcedure::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New(Procedure::GetClass(), "NativeProcedure");
  ASSERT(kClass);
}

auto NativeProcedure::ReturnValue(Object* rhs) const -> bool {
  ASSERT(rhs);
  GetRuntime()->Push(rhs);
  return true;
}

auto NativeProcedure::Apply(const std::vector<Object*>& rhs) const -> bool {
  const auto result = ApplyProcedure(rhs);
  return result;
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