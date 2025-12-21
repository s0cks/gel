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
NativeFnList NativeFn::all_{};

void NativeFn::Init() {
  using namespace proc;
  InitClass();
  InitNatives();
}

void NativeFn::Register(NativeFn* native) {
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

auto NativeFn::Find(const std::string& name) -> NativeFn* {
  ASSERT(!name.empty());
  for (const auto& native : all_) {
    ASSERT(native);
    const auto symbol = native->GetSymbol()->GetFullyQualifiedName();
    if (name == symbol)
      return native;
  }
  return nullptr;
}

auto NativeFn::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Fn::VisitPointers(vis))
    return false;
  // TODO: visit entry_?
  return true;
}

auto NativeFn::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Fn::VisitPointerPointers(vis))
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

auto NativeFn::GetHashCode() const -> HashCode {
  return Fn::GetHashCode();
}

auto NativeFn::Find(Symbol* symbol) -> NativeFn* {
  ASSERT(symbol);
  for (const auto& native : all_) {
    ASSERT(native);
    if (native->GetSymbol()->Equals(symbol))
      return native;
  }
  return nullptr;
}

auto NativeFn::FindOrCreate(Symbol* symbol) -> NativeFn* {
  ASSERT(symbol);
  for (const auto& native : all_) {
    ASSERT(native);
    if (native->GetSymbol()->Equals(symbol))
      return native;
  }
  const auto native = new NativeFn(symbol);
  ASSERT(native);
  NativeFn::Register(native);
  return native;
}

void NativeFn::Link(Symbol* symbol, NativeFnEntry* entry) {
  ASSERT(symbol);
  ASSERT(entry);
  LOG_IF(FATAL, entry->IsBound()) << "cannot rebind " << (*entry);
  NativeFn* native = FindOrCreate(symbol);
  ASSERT(native);
  LOG_IF(FATAL, native->HasEntry()) << "cannot relink " << native->ToString();
  native->SetEntry(entry);
  entry->SetNative(native);
}

auto NativeFn::New(const ObjectList& args) -> NativeFn* {
  NOT_IMPLEMENTED(FATAL);
}

auto NativeFn::CreateClass() -> Class* {
  return Class::New(Fn::GetClass(), kClassName);
}

auto NativeFn::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsNativeFn())
    return false;
  return GetSymbol()->Equals(rhs->AsNativeFn()->GetSymbol());
}

auto NativeFn::ToString() const -> std::string {
  ToStringHelper<NativeFn> helper;
  helper.AddField("symbol", GetSymbol()->GetFullyQualifiedName());
  helper.AddField("args", GetArgs());
  helper.AddField("docs", GetDocstring());
  return helper;
}
}  // namespace gel
