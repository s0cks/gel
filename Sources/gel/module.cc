#include "module.h"

#include "array.h"
#include "common.h"
#include "expr/expression.h"
#include "macro.h"
#include "native_procedure.h"
#include "parser.h"
#include "platform.h"
#include "pointer.h"
#include "to_string_helper.h"

namespace gel {
static Array<Module*>* modules_ = nullptr;

static inline auto Register(Module* m) -> Module* {
  ASSERT(m);
  ASSERT(modules_);
  modules_->Push(m);
  return m;
}

void Module::GetAllLoadedModules(std::vector<Module*>& results) {
  for (auto idx = 0; idx < modules_->GetLength(); idx++) {
    const auto m = modules_->Get(idx);
    ASSERT(m);
    results.push_back(m);
  }
}

auto Module::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Module::CreateConstructor(Module* rhs, expr::SeqExpr* body) -> Lambda* {
  ASSERT(rhs);
  const auto init = Lambda::New();
  init->SetSymbol(rhs->GetSymbol());
  init->SetBody(body);
  init->SetArgs(Array<Argument*>::New(1));
  init->SetScope(LocalScope::NewWithThis(rhs));
  return init;
}

auto Module::Init(Runtime* runtime) -> bool {
  ASSERT(runtime);
  ASSERT(!IsInitialized() && HasInit());
  runtime->InvokeConstructor(this);
  for (auto idx = 0; idx < namespaces_->GetLength(); idx++) {
    const auto ns = namespaces_->Get(idx);
    ASSERT(ns);
    ns->Init(runtime);
  }
  SetInitialized(true);
  return IsInitialized();
}

auto Module::Find(const std::string& name) -> Module* {
  return modules_->FindIf(IsNamed<Module>(name));
}

void Module::AddChild(Object* rhs) {
  ASSERT(rhs);
  if (rhs->IsNamespace()) {
    namespaces_->Push(rhs->AsNamespace());
  }
}

auto Module::FindOrLoad(const std::string& name) -> Module* {
  const auto m = modules_->FindIf(IsNamed<Module>(name));
  if (m == nullptr) {
    const auto home = GetHomeEnvVar();
    if (!home)
      return nullptr;
    const auto new_module = Module::LoadFrom(fmt::format("{}/lib/{}", (*home.value()), name));
    LOG_IF(FATAL, !new_module) << "failed to create new module from: " << name;
    GetRuntime()->GetInitScope()->AddAll(new_module->GetScope());
    if (new_module->HasInit())
      LOG_IF(FATAL, !new_module->Init(GetRuntime())) << "failed to initialize the _kernel Module: " << new_module;
    return new_module;
  }
  return m;
}

auto Module::New(Symbol* name, LocalScope* scope) -> Module* {
  ASSERT(name);
  ASSERT(scope);
  return Register(new Module(name, scope));
}

auto Module::LoadFrom(const std::filesystem::path& abs_path) -> Module* {
  DVLOG(100) << "loading Module from: " << abs_path << "....";
  return Parser::ParseModuleFrom(abs_path);
}

auto Module::ToString() const -> std::string {
  ToStringHelper<Module> helper{};
  helper.AddField("symbol", GetSymbol()->GetFullyQualifiedName());
  return helper;
}

auto Module::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= *(GetSymbol());
  return hash;
}

auto Module::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsModule())
    return false;
  const auto other = rhs->AsModule();
  ASSERT(other);
  return GetSymbol()->Equals(other->GetSymbol());
}

auto Module::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Visit(init_, *vis))
    return false;
  if (!Visit(namespaces_, *vis))
    return false;
  if (!Visit(GetSymbol(), *vis))
    return false;
  if (!Visit(GetInitialized(), *vis))
    return false;
  if (!Visit(GetInit(), *vis))
    return false;
  return true;
}

auto Module::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!VisitPointerPointer(vis, &namespaces_))
    return false;
  if (!VisitPointerPointer(vis, &init_))
    return false;
  if (!VisitPointerPointer(vis, &scope_))
    return false;

  auto name = GetSymbol()->raw_ptr();
  if (!vis->Visit(&name))
    return false;
  if (!GetSymbol()->raw_ptr()->Equals(name))
    SetSymbol(name->As<Symbol>());

  auto kernel = GetKernelField()->raw_ptr();
  if (!vis->Visit(&kernel))
    return false;
  if (!GetKernelField()->raw_ptr()->Equals(kernel))
    SetKernelField(kernel->As<Bool>());

  auto initialized = GetInitialized()->raw_ptr();
  if (!vis->Visit(&initialized))
    return false;
  if (!GetInitialized()->raw_ptr()->Equals(initialized))
    SetInitialized(initialized->As<Bool>());
  return true;
}

Field* Module::kKernelField = nullptr;
Field* Module::kFieldInitialized = nullptr;
Field* Module::kSymbolField = nullptr;
auto Module::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  const auto cls = Class::New(Object::GetClass(), "Module");
  ASSERT(cls);
  kSymbolField = cls->AddField("symbol");
  ASSERT(kSymbolField);
  kKernelField = cls->AddField("kernel");
  ASSERT(kKernelField);
  kFieldInitialized = cls->AddField("initialized");
  ASSERT(kFieldInitialized);
  return cls;
}

auto Module::New(const ObjectList& args) -> Module* {
  NOT_IMPLEMENTED(FATAL);
}

auto Module::VisitAllModules(ModuleVisitor* vis) -> bool {
  ASSERT(vis);
  for (auto idx = 0; idx < modules_->GetLength(); idx++) {
    const auto m = modules_->Get(idx);
    ASSERT(m);
    if (!vis->Visit(m))
      return false;
  }
  return true;
}

auto Module::VisitAllModulePointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  return modules_->VisitPointers(vis);
}

auto Module::VisitAllModulePointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!modules_->VisitPointerPointers(vis))
    return false;
  if (!VisitPointerPointer(vis, &modules_))
    return false;
  if (!VisitPointerPointer(vis, &kSymbolField))
    return false;
  if (!VisitPointerPointer(vis, &kKernelField))
    return false;
  if (!VisitPointerPointer(vis, &kFieldInitialized))
    return false;
  return true;
}

#define INIT_MODULE_NATIVE(Name) InitNative<module_##Name>()

void Module::Init() {
  InitClass();
  ASSERT(modules_ == nullptr);
  modules_ = Array<Module*>::New();
  ASSERT(modules_);

  using namespace proc;
  InitNative<gel_get_modules>();
  InitNative<gel_get_module>();
  INIT_MODULE_NATIVE(is_kernel);
  INIT_MODULE_NATIVE(get_namespaces);
}

#undef INIT_MODULE_NATIVE

namespace proc {
NATIVE_PROCEDURE_F(gel_get_module) {
  REQUIRED_NATIVE_ARG(0, Symbol, symbol);
  return Return(Module::Find(symbol->GetFullyQualifiedName()));
}

NATIVE_PROCEDURE_F(gel_get_modules) {
  std::vector<Module*> modules{};
  Module::GetAllLoadedModules(modules);
  return Return(ToList((const ObjectList&)modules));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

#define MODULE_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(module_##Name)

MODULE_PROCEDURE_F(is_kernel) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  Module* m = nullptr;
  if (value->IsSymbol()) {
    m = Module::Find(value->AsSymbol()->GetFullyQualifiedName());
  } else if (value->IsModule()) {
    m = value->AsModule();
  }
  if (!m) {
    std::stringstream ss;
    ss << "failed to find Module for: " << value->ToString();
    return ThrowError(ss);
  }
  return ReturnBool(m->IsKernel());
}

MODULE_PROCEDURE_F(get_namespaces) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  Module* m = nullptr;
  if (value->IsSymbol()) {
    m = Module::Find(value->AsSymbol()->GetFullyQualifiedName());
  } else if (value->IsModule()) {
    m = value->AsModule();
  }
  if (!m) {
    std::stringstream ss;
    ss << "failed to find Module for: " << value->ToString();
    return ThrowError(ss);
  }
  const auto namespaces = m->GetNamespaces();
  ASSERT(namespaces);

  Object* result = Nil::Get();
  ASSERT(result);
  for (auto idx = 0; idx < namespaces->GetLength(); idx++) {
    const auto ns = namespaces->Get(idx);
    ASSERT(ns);
    result = Cons(ns, result);
    ASSERT(result);
  }
  return Return(result);
}

#undef MODULE_PROCEDURE_F

}  // namespace proc
}  // namespace gel
