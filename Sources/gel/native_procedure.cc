#include "gel/native_procedure.h"

#include "gel/common.h"
#include "gel/runtime.h"
#include "gel/type.h"

namespace gel {
NativeProcedureList NativeProcedure::all_{};

void NativeProcedure::Register(NativeProcedure* native) {
  ASSERT(native);
  all_.push_back(native);
}

auto NativeProcedure::Find(const std::string& name) -> NativeProcedure* {
  ASSERT(!name.empty());
  for (const auto& native : all_) {
    ASSERT(native);
    const auto symbol = native->GetSymbol()->Get();
    if (name == symbol)
      return native;
  }
  return nullptr;
}

auto NativeProcedure::Find(Symbol* symbol) -> NativeProcedure* {
  ASSERT(symbol);
  for (const auto& native : all_) {
    ASSERT(native);
    if (native->GetSymbol()->Equals(symbol))
      return native;
  }
  return nullptr;
}

auto NativeProcedure::New(const ObjectList& args) -> NativeProcedure* {
  NOT_IMPLEMENTED(FATAL);
}

auto NativeProcedure::CreateClass() -> Class* {
  return Class::New(Procedure::GetClass(), kClassName);
}

auto NativeProcedure::Return(Object* rhs) const -> bool {
  ASSERT(rhs);
  GetRuntime()->Push(rhs);
  return DoNothing();
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

}  // namespace gel