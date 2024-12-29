#include "gel/native_procedure.h"

#include "gel/common.h"
#include "gel/local.h"
#include "gel/natives.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"

namespace gel {
auto NativeProcedureEntry::Return(Object* rhs) const -> bool {
  ASSERT(rhs);
  const auto stack = GetRuntime()->GetOperationStack();
  ASSERT(stack);
  stack->Push(rhs);
  return DoNothing();
}

NativeProcedureList NativeProcedure::all_{};

void NativeProcedure::Init() {
  using namespace proc;
  InitClass();
  InitNatives();
}

void NativeProcedure::Register(NativeProcedure* native) {
  ASSERT(native);
  const auto scope = GetRuntime()->GetInitScope();
  ASSERT(scope);
  LocalVariable* local = nullptr;
  if (!scope->Lookup(native->GetSymbol(), &local, false)) {
    local = LocalVariable::New(scope, native->GetSymbol(), native);
    ASSERT(local);
    LOG_IF(FATAL, !scope->Add(local)) << "failed to add register native " << native << " in global scope.";
  }
  ASSERT(local);
  if (!local->HasValue())
    local->SetValue(native);
  all_.push_back(native);
}

auto NativeProcedure::Find(const std::string& name) -> NativeProcedure* {
  ASSERT(!name.empty());
  for (const auto& native : all_) {
    ASSERT(native);
    const auto symbol = native->GetSymbol()->GetFullyQualifiedName();
    if (name == symbol)
      return native;
  }
  return nullptr;
}

auto NativeProcedure::HashCode() const -> uword {
  return Procedure::HashCode();
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

auto NativeProcedure::FindOrCreate(Symbol* symbol) -> NativeProcedure* {
  ASSERT(symbol);
  for (const auto& native : all_) {
    ASSERT(native);
    if (native->GetSymbol()->Equals(symbol))
      return native;
  }
  const auto native = new NativeProcedure(symbol);
  ASSERT(native);
  NativeProcedure::Register(native);
  return native;
}

void NativeProcedure::Link(Symbol* symbol, NativeProcedureEntry* entry) {
  ASSERT(symbol);
  ASSERT(entry);
  LOG_IF(FATAL, entry->IsBound()) << "cannot rebind " << (*entry);
  NativeProcedure* native = FindOrCreate(symbol);
  ASSERT(native);
  LOG_IF(FATAL, native->HasEntry()) << "cannot relink " << native->ToString();
  native->SetEntry(entry);
  entry->SetNative(native);
}

auto NativeProcedure::New(const ObjectList& args) -> NativeProcedure* {
  NOT_IMPLEMENTED(FATAL);
}

auto NativeProcedure::CreateClass() -> Class* {
  return Class::New(Procedure::GetClass(), kClassName);
}

auto NativeProcedure::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsNativeProcedure())
    return false;
  return GetSymbol()->Equals(rhs->AsNativeProcedure()->GetSymbol());
}

auto NativeProcedure::ToString() const -> std::string {
  ToStringHelper<NativeProcedure> helper;
  helper.AddField("symbol", GetSymbol()->GetFullyQualifiedName());
  helper.AddField("args", GetArgs());
  if (HasDocs())
    helper.AddField("docs", GetDocs());
  return helper;
}
}  // namespace gel