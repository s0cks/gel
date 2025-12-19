#include "native_procedure.h"

#include "argument.h"
#include "common.h"
#include "local.h"
#include "natives.h"
#include "pointer.h"
#include "runtime.h"
#include "to_string_helper.h"
#include "type.h"

namespace gel {
auto NativeProcedureEntry::Return(Object* rhs) const -> bool {
  ASSERT(rhs);
  GetRuntime()->GetCallStack()->SetReturnAddress(rhs->GetStartingAddress());
  return DoNothing();
}

auto NativeProcedure::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto NativeProcedureEntry::ThrowNotImplementedError() const -> bool {
  std::stringstream ss;
  if (HasNative()) {
    ss << "NativeProcedure `" << GetNative()->GetSymbol()->GetFullyQualifiedName() << "` is ";
  }
  ss << "not implemented!";
  return ThrowError(ss);
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

auto NativeProcedure::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Procedure::VisitPointers(vis))
    return false;
  // TODO: visit entry_?
  return true;
}

auto NativeProcedure::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Procedure::VisitPointerPointers(vis))
    return false;
  if (HasDocstring()) {
    auto docs = GetDocstring()->raw_ptr();
    if (!vis->Visit(&docs))
      return false;
    if (!GetDocstring()->raw_ptr()->Equals(docs))
      SetDocstring(docs->As<String>());
  }
  if (HasArgs()) {
    auto args = GetArgs()->raw_ptr();
    if (!vis->Visit(&args))
      return false;
    if (!GetDocstring()->raw_ptr()->Equals(args))
      SetArgs(args->As<Array<Argument*>>());
  }
  return true;
}

auto NativeProcedure::GetHashCode() const -> HashCode {
  return Procedure::GetHashCode();
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
  helper.AddField("docs", GetDocstring());
  return helper;
}
}  // namespace gel
