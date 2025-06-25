#include "gel/namespace.h"

#include <algorithm>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/local.h"
#include "gel/macro.h"
#include "gel/native_procedure.h"
#include "gel/pointer.h"
#include "gel/procedure.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"

namespace gel {
static Array<Namespace*>* namespaces_ = nullptr;

auto Namespace::IsKernelNamespace() const -> bool {
  return GetName() == "_kernel";
}

void Namespace::AddChild(Object* rhs) {
  ASSERT(rhs);
  if (rhs->IsProcedure()) {
    const auto procedure = rhs->AsProcedure();
    ASSERT(procedure);
    procedures_->Push(procedure);
    procedure->SetOwner(this);
    return;
  } else if (rhs->IsMacro()) {
    const auto macro = rhs->AsMacro();
    ASSERT(macro);
    macros_->Push(macro);
    macro->SetOwner(this);
    return;
  }
  DLOG(ERROR) << "cannot add " << rhs->ToString() << " to " << ToString();
}

auto Namespace::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!VisitPointerPointer(vis, &owner_))
    return false;
  if (!VisitPointerPointer(vis, &symbol_))
    return false;
  if (!VisitPointerPointer(vis, &scope_))
    return false;
  if (!VisitPointerPointer(vis, &docs_))
    return false;
  return true;
}

auto Namespace::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Namespace::HashCode() const -> uword {
  uword hash = 0;
  CombineHash(hash, GetSymbol()->HashCode());
  return hash;
}

auto Namespace::HasSymbol(const std::string& rhs) const -> bool {
  ASSERT(!rhs.empty());
  LocalVariable* local = nullptr;
  return GetScope()->Lookup(rhs, &local, false);
}

auto Namespace::Get(const std::string& rhs) const -> Object* {
  ASSERT(!rhs.empty());
  LocalVariable* local = nullptr;
  if (!GetScope()->Lookup(rhs, &local, false))
    return nullptr;
  ASSERT(local);
  return local->GetValue();
}

auto Namespace::Get(Symbol* rhs) const -> Object* {
  ASSERT(rhs);
  LocalVariable* local = nullptr;
  if (!GetScope()->Lookup(rhs, &local, false))
    return nullptr;
  ASSERT(local);
  return local->GetValue();
}

auto Namespace::HasSymbol(Symbol* rhs) const -> bool {
  ASSERT(rhs);
  LocalVariable* local = nullptr;
  return GetScope()->Lookup(rhs, &local, false);
}

auto Namespace::GetName() const -> const std::string& {
  return GetSymbol()->GetSymbolName();
}

auto Namespace::VisitAllNamespaces(NamespaceVisitor* vis) -> bool {
  ASSERT(vis);
  ASSERT(namespaces_);
  for (auto idx = 0; idx < namespaces_->GetLength(); idx++) {
    const auto ns = namespaces_->Get(idx);
    ASSERT(ns);
    if (!vis->Visit(ns))
      return false;
  }
  return true;
}

auto Namespace::FindNamespace(const Namespace::Predicate& filter) -> Namespace* {
  ASSERT(namespaces_);
  for (auto idx = 0; idx < namespaces_->GetLength(); idx++) {
    const auto ns = namespaces_->Get(idx);
    ASSERT(ns);
    if (filter(ns))
      return ns;
  }
  return nullptr;
}

auto Namespace::CreateSymbol(const std::string& rhs) -> Symbol* {
  ASSERT(!rhs.empty());
  if (IsKernelNamespace())
    return Symbol::New(rhs);
  if (Contains(rhs, '/')) {
    const auto last = rhs.find_last_of('/');
    ASSERT(last != std::string::npos);
    return Symbol::New(GetSymbol()->GetSymbolName(), rhs.substr(last + 1));
  }
  return Symbol::New(GetSymbol()->GetSymbolName(), rhs);
}

auto Namespace::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Namespace");
}

auto Namespace::New(Symbol* symbol, LocalScope* scope) -> Namespace* {
  ASSERT(symbol);
  ASSERT(scope);
  const auto ns = new Namespace(symbol, scope);
  ASSERT(ns);
  namespaces_->Push(ns);
  return ns;
}

auto Namespace::New(const ObjectList& args) -> Namespace* {
  LOG(FATAL) << "cannot create a new Namespace";
}

auto Namespace::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsNamespace())
    return false;
  const auto other = rhs->AsNamespace();
  ASSERT(other);
  return GetSymbol()->Equals(other->GetSymbol());
}

auto Namespace::ToString() const -> std::string {
  ToStringHelper<Namespace> helper;
  helper.AddField("symbol", GetSymbol()->GetFullyQualifiedName());
  helper.AddField("scope", GetScope());
  return helper;
}

auto Namespace::InitNamespace() -> Namespace* {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  runtime->InvokeConstructor(this);
  return this;
}

auto Namespace::FindMacro(const std::string& name) -> Macro* {
  ASSERT(!name.empty());
  return macros_->FindIf(Macro::IsNamed(name));
}

auto Namespace::FindProcedure(const std::string& name) -> Procedure* {
  ASSERT(!name.empty());
  return procedures_->FindIf(Procedure::IsNamed(name));
}

auto Namespace::FindNativeProcedure(const std::string& name) -> NativeProcedure* {
  const auto proc = FindProcedure(name);
  return proc && proc->IsNativeProcedure() ? proc->AsNativeProcedure() : nullptr;
}

auto Namespace::FindLambda(const std::string& name) -> Lambda* {
  const auto proc = FindProcedure(name);
  return proc && proc->IsLambda() ? proc->AsLambda() : nullptr;
}

auto Namespace::VisitAllMacros(MacroVisitor* vis) const -> bool {
  ASSERT(vis);
  for (auto idx = 0; idx < macros_->GetLength(); idx++) {
    const auto macro = macros_->Get(idx);
    ASSERT(macro);
    if (!vis->Visit(macro))
      return false;
  }
  return true;
}

auto Namespace::VisitAllProcedures(ProcedureVisitor* vis) const -> bool {
  ASSERT(vis);
  for (auto idx = 0; idx < procedures_->GetLength(); idx++) {
    const auto proc = procedures_->Get(idx);
    ASSERT(proc);
    if (!vis->Visit(proc))
      return false;
  }
  return true;
}

auto Namespace::VisitAllLambdaProcedures(ProcedureVisitor* vis) const -> bool {
  ASSERT(vis);
  for (auto idx = 0; idx < procedures_->GetLength(); idx++) {
    const auto proc = procedures_->Get(idx);
    ASSERT(proc);
    if (!proc->IsLambda())
      continue;
    if (!vis->Visit(proc))
      return false;
  }
  return true;
}

auto Namespace::VisitAllNativeProcedures(ProcedureVisitor* vis) const -> bool {
  ASSERT(vis);
  for (auto idx = 0; idx < procedures_->GetLength(); idx++) {
    const auto proc = procedures_->Get(idx);
    ASSERT(proc);
    if (!proc->IsNative())
      continue;
    if (!vis->Visit(proc))
      return false;
  }
  return true;
}

void Namespace::Init() {
  namespaces_ = Array<Namespace*>::New();
  ASSERT(namespaces_);
  InitClass();
  using namespace proc;
  InitNative<gel_get_namespace>();
  InitNative<gel_get_namespaces>();

  InitNative<namespace_get_symbol>();
  InitNative<namespace_get_owner>();
  InitNative<namespace_get_macros>();
  InitNative<namespace_get_lambdas>();
  InitNative<namespace_get_native_procedures>();
  InitNative<namespace_get_procedures>();
}

auto Namespace::CreateConstructor(Namespace* ns, expr::SeqExpr* body) -> Constructor* {
  const auto init = Constructor::New(Symbol::New(ns->GetName()), body);
  init->SetArgs(Array<Argument*>::New(1));
  init->SetScope(LocalScope::NewWithThis(ns));
  return init;
}

namespace proc {
NATIVE_PROCEDURE_F(gel_get_namespace) {
  NativeArgument<0, Symbol> symbol(args);
  if (!symbol)
    return Throw(symbol);
  const auto ns = Namespace::FindNamespace(symbol);
  if (!ns) {
    std::stringstream ss;
    ss << "failed to find Namespace for symbol: " << symbol->GetFullyQualifiedName();
    return ThrowError(ss);
  }
  return Return(ns);
}

NATIVE_PROCEDURE_F(gel_get_namespaces) {
  Object* result = Null();
  NamespaceVisitorWrapper vis([&result](Namespace* ns) {
    ASSERT(ns);
    result = Cons(ns, result);
    ASSERT(result);
    return true;
  });
  if (!Namespace::VisitAllNamespaces(&vis))
    return ThrowError("failed to visit Namespaces");
  return Return(result);
}

#define NAMESPACE_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(namespace_##Name)

NAMESPACE_PROCEDURE_F(get_owner) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  Namespace* target = nullptr;
  if (value->IsNamespace()) {
    target = value->AsNamespace();
  } else if (value->IsSymbol()) {
    target = Namespace::FindNamespace(value->AsSymbol());
  }
  if (!target) {
    std::stringstream ss;
    ss << "failed to find Namespace: " << target;
    return ThrowError(ss);
  }
  ASSERT(target);
  return Return(target->GetOwner());
}

NAMESPACE_PROCEDURE_F(get_symbol) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  Namespace* target = nullptr;
  if (value->IsNamespace()) {
    target = value->AsNamespace();
  } else if (value->IsSymbol()) {
    target = Namespace::FindNamespace(value->AsSymbol());
  }
  if (!target) {
    std::stringstream ss;
    ss << "failed to find Namespace: " << target;
    return ThrowError(ss);
  }
  ASSERT(target);
  return Return(target->GetSymbol());
}

NAMESPACE_PROCEDURE_F(get_macros) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  Namespace* target = nullptr;
  if (value->IsNamespace()) {
    target = value->AsNamespace();
  } else if (value->IsSymbol()) {
    target = Namespace::FindNamespace(value->AsSymbol());
  }
  if (!target) {
    std::stringstream ss;
    ss << "failed to find Namespace: " << target;
    return ThrowError(ss);
  }
  ASSERT(target);
  Object* result = Null();
  MacroVisitorWrapper visitor([&result](Macro* macro) {
    ASSERT(macro);
    result = Cons(macro, result);
    return true;
  });
  if (!target->VisitAllMacros(&visitor)) {
    std::stringstream ss;
    ss << "failed to get Macros for: " << target->ToString();
    return ThrowError(ss);
  }
  return Return(result);
}

NAMESPACE_PROCEDURE_F(get_procedures) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  Namespace* target = nullptr;
  if (value->IsNamespace()) {
    target = value->AsNamespace();
  } else if (value->IsSymbol()) {
    target = Namespace::FindNamespace(value->AsSymbol());
  }
  if (!target) {
    std::stringstream ss;
    ss << "failed to find Namespace: " << target;
    return ThrowError(ss);
  }
  ASSERT(target);
  Object* result = Null();
  ProcedureVisitorWrapper visitor([&result](Procedure* macro) {
    ASSERT(macro);
    result = Cons(macro, result);
    return true;
  });
  if (!target->VisitAllProcedures(&visitor)) {
    std::stringstream ss;
    ss << "failed to get Procedures for: " << target->ToString();
    return ThrowError(ss);
  }
  return Return(result);
}

NAMESPACE_PROCEDURE_F(get_lambdas) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  Namespace* target = nullptr;
  if (value->IsNamespace()) {
    target = value->AsNamespace();
  } else if (value->IsSymbol()) {
    target = Namespace::FindNamespace(value->AsSymbol());
  }
  if (!target) {
    std::stringstream ss;
    ss << "failed to find Namespace: " << target;
    return ThrowError(ss);
  }
  ASSERT(target);
  Object* result = Null();
  ProcedureVisitorWrapper visitor([&result](Procedure* macro) {
    ASSERT(macro);
    result = Cons(macro, result);
    return true;
  });
  if (!target->VisitAllLambdaProcedures(&visitor)) {
    std::stringstream ss;
    ss << "failed to get all Lambdas for: " << target->ToString();
    return ThrowError(ss);
  }
  return Return(result);
}

NAMESPACE_PROCEDURE_F(get_native_procedures) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  Namespace* target = nullptr;
  if (value->IsNamespace()) {
    target = value->AsNamespace();
  } else if (value->IsSymbol()) {
    target = Namespace::FindNamespace(value->AsSymbol());
  }
  if (!target) {
    std::stringstream ss;
    ss << "failed to find Namespace: " << target;
    return ThrowError(ss);
  }
  ASSERT(target);
  Object* result = Null();
  ProcedureVisitorWrapper visitor([&result](Procedure* macro) {
    ASSERT(macro);
    result = Cons(macro, result);
    return true;
  });
  if (!target->VisitAllNativeProcedures(&visitor)) {
    std::stringstream ss;
    ss << "failed to get NativeProcedures for: " << target->ToString();
    return ThrowError(ss);
  }
  return Return(result);
}

#undef NAMESPACE_PROCEDURE_F
}  // namespace proc
}  // namespace gel