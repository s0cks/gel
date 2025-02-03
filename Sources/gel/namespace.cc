#include "gel/namespace.h"

#include <algorithm>

#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/local.h"
#include "gel/pointer.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"

namespace gel {
static Array<Namespace*>* namespaces_ = nullptr;

auto Namespace::IsKernelNamespace() const -> bool {
  return GetName() == "_kernel";
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
  if (HasInit()) {
    const auto runtime = GetRuntime();
    ASSERT(runtime);
    runtime->Call(GetInit(), {this});
  }
  return this;
}

auto Namespace::CreateInit(const expr::ExpressionList& body) -> Procedure* {
  const auto symbol = Symbol::New(GetSymbol()->GetNamespace(), "init");
  ASSERT(symbol);
  const auto args = Array<Argument*>::New();
  ASSERT(args);
  const auto init = Lambda::New(symbol, args, body);
  ASSERT(init);
  const auto scope = LocalScope::New();
  ASSERT(scope);
  const auto self = LocalVariable::New(scope, "this", this);
  LOG_IF(FATAL, !scope->Add(self)) << "failed to add " << (*self) << " to scope.";
  init->SetScope(scope);
  init_ = init;
  return init;
}

void Namespace::Init() {
  namespaces_ = Array<Namespace*>::New();
  ASSERT(namespaces_);
  InitClass();
  InitNative<proc::gel_get_namespace>();
  InitNative<proc::gel_get_namespaces>();
}

namespace proc {
#define NAMESPACE_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(namespace_##Name)

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

#undef NAMESPACE_PROCEDURE_F
}  // namespace proc
}  // namespace gel