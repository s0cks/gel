#include "gel/module.h"

#include "gel/array.h"
#include "gel/common.h"
#include "gel/macro.h"
#include "gel/parser.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/to_string_helper.h"

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

auto Module::CreateInitFunc(const expr::ExpressionList& body) -> Lambda* {
  ASSERT(!body.empty());
  const auto args = Array<Argument*>::New(1);
  ASSERT(args);
  const auto init = Lambda::New(args, body);
  const auto scope = LocalScope::New();
  ASSERT(scope);
  const auto self = LocalVariable::New(scope, "this", this);
  LOG_IF(FATAL, !scope->Add(self)) << "failed to add " << (*self) << " to scope.";
  init->SetScope(scope);
  SetInit(init);
  return init;
}

auto Module::Init(Runtime* runtime) -> bool {
  ASSERT(runtime);
  ASSERT(!IsInitialized() && HasInit());
  runtime->Call(GetInit(), {this});

  for (auto idx = 0; idx < namespaces_->GetLength(); idx++) {
    const auto ns = namespaces_->Get(idx);
    ASSERT(ns);
    if (ns->HasInit()) {
      runtime->Call(ns->GetInit(), {ns});
      DLOG(INFO) << ns->InitNamespace()->ToString() << " initialized!";
    }
  }

  SetInitialized(true);
  return IsInitialized();
}

auto Module::Find(const std::string& name) -> Module* {
  return modules_->FindIf(IsNamed(name));
}

void Module::AddChild(Object* rhs) {
  ASSERT(rhs);
  if (rhs->IsNamespace()) {
    namespaces_->Push(rhs->AsNamespace());
  }
}

auto Module::FindOrLoad(const std::string& name) -> Module* {
  const auto m = modules_->FindIf(IsNamed(name));
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

auto Module::New(String* name, LocalScope* scope) -> Module* {
  ASSERT(name);
  ASSERT(scope);
  const auto m = new Module(name, scope);
  ASSERT(m && kFieldInitialized->GetOffset() > 0);
  m->SetField(kFieldInitialized, Bool::False());
  return Register(m);
}

auto Module::LoadFrom(const std::filesystem::path& abs_path) -> Module* {
  DVLOG(100) << "loading Module from: " << abs_path << "....";
  return Parser::ParseModuleFrom(abs_path);
}

auto Module::ToString() const -> std::string {
  ToStringHelper<Module> helper;
  helper.AddField("name", GetName()->Get());
  return helper;
}

auto Module::HashCode() const -> uword {
  uword hash = 0;
  CombineHash(hash, GetName()->Get());
  return hash;
}

auto Module::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsModule())
    return false;
  const auto other = rhs->AsModule();
  ASSERT(other);
  return GetName()->Equals(other->GetName());
}

auto Module::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!vis->Visit(GetName()))
    return false;
  if (!vis->Visit(GetInitialized()))
    return false;
  if (!vis->Visit(GetInit()))
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

  auto name = GetName()->raw_ptr();
  if (!vis->Visit(&name))
    return false;
  if (!GetName()->raw_ptr()->Equals(name))
    SetName(name->As<String>());

  auto initialized = GetInitialized()->raw_ptr();
  if (!vis->Visit(&initialized))
    return false;
  if (!GetInitialized()->raw_ptr()->Equals(initialized))
    SetInitialized(initialized->As<Bool>());
  return true;
}

Field* Module::kFieldInitialized = nullptr;
Field* Module::kNameField = nullptr;
auto Module::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  const auto cls = Class::New(Object::GetClass(), "Module");
  ASSERT(cls);
  kNameField = cls->AddField("name");
  ASSERT(kNameField);
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
  if (!VisitPointerPointer(vis, &kNameField))
    return false;
  if (!VisitPointerPointer(vis, &kFieldInitialized))
    return false;
  return true;
}

void Module::Init() {
  InitClass();
  ASSERT(modules_ == nullptr);
  modules_ = Array<Module*>::New();
  ASSERT(modules_);
}
}  // namespace gel