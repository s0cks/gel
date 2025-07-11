#include "gel/native_procedure.h"

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/local.h"
#include "gel/natives.h"
#include "gel/pointer.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"

namespace gel {
auto NativeFnEntry::Return(Object* rhs) const -> bool {
  ASSERT(rhs);
  GetRuntime()->GetCallStack()->SetReturnAddress(rhs->GetStartingAddress());
  return DoNothing();
}

auto NativeFn::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto NativeFnEntry::ThrowNotImplementedError() const -> bool {
  std::stringstream ss;
  if (HasNative()) {
    ss << "NativeFn `" << GetNative()->GetSymbol()->GetFullyQualifiedName() << "` is ";
  }
  ss << "not implemented!";
  return ThrowError(ss);
}

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

template <StringLike S>
auto NativeFn::Find(S symbol) -> NativeFn* {
  ASSERT(!symbol.empty());
  for (const auto& fn : all_) {
    if (fn->GetSymbol()->Equals(symbol))
      return fn;
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
  // TODO: @s0cks visit entry_?
  return true;
}

auto NativeFn::GetHashCode() const -> HashCode {
  return Fn::GetHashCode();
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
  const auto other = rhs->AsNativeFn();
  return GetSymbol()->Equals(other->GetSymbol());
}

auto NativeFn::ToString() const -> std::string {
  ToStringHelper<NativeFn> helper{};
  helper.AddSymbolField(*GetSymbol());
  helper.AddField("args", GetArgs());
  helper.AddField("docs", GetDocstring());
  return helper;
}

namespace procs {
// _DECLARE_NATIVE_PROCEDURE(gel_get_native_fn, "gel/get-native-fn");
// _DECLARE_NATIVE_PROCEDURE(gel_get_native_fns, "gel/get-native-fns");
// _DECLARE_NATIVEFN_FN(get_owner, "get-owner");
// _DECLARE_NATIVEFN_FN(get_symbol, "get-symbol");
// _DECLARE_NATIVEFN_FN(get_docs, "get-docs");
}  // namespace procs
}  // namespace gel