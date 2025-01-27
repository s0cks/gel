#include "gel/module.h"

#include "gel/common.h"
#include "gel/macro.h"
#include "gel/parser.h"
#include "gel/to_string_helper.h"

namespace gel {
std::vector<Pointer*> modules_{};

static inline auto Register(Module* m) -> Module* {
  ASSERT(m);
  modules_.push_back(m->raw_ptr());
  return m;
}

void Module::GetAllLoadedModules(std::vector<Module*>& modules) {
  for (const auto& m : modules_) {
    modules.push_back(m->As<Module>());
  }
}

auto Module::CreateInitFunc(const expr::ExpressionList& body) -> Lambda* {
  ASSERT(!body.empty());
  const ArgumentSet args = {Argument(0, "this", false, false)};
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
  ASSERT(!IsInitialized());
  runtime->Call(GetInit(), {this});
  return IsInitialized();
}

void Module::Append(Macro* macro) {
  ASSERT(macro);
  macros_.push_back(macro);
  macro->SetOwner(this);
}

void Module::Append(Namespace* ns) {
  ASSERT(ns);
  namespaces_.push_back(ns);
  if (scope_ && ns->GetScope())
    scope_->Add(ns->GetScope());
}

auto Module::IsLoaded(const std::string& name) -> bool {
  const auto filter = IsNamed(name);
  const auto m = std::ranges::find_if(modules_, [&filter](Pointer* ptr) {
    ASSERT(ptr && ptr->GetObjectPointer());
    const auto m = ptr->As<Module>();
    return filter(m);
  });
  return m != std::end(modules_);
}

auto Module::Find(const std::string& name) -> Module* {
  const auto filter = IsNamed(name);
  const auto m = std::ranges::find_if(modules_, [&filter](Pointer* ptr) {
    ASSERT(ptr && ptr->GetObjectPointer());
    const auto m = ptr->As<Module>();
    return filter(m);
  });
  if (m == std::end(modules_) || !(*m)->GetObjectPointer())
    return nullptr;
  return (*m)->As<Module>();
}

auto Module::FindOrLoad(const std::string& name) -> Module* {
  const auto filter = IsNamed(name);
  const auto m = std::ranges::find_if(modules_, [&filter](Pointer* ptr) {
    ASSERT(ptr && ptr->GetObjectPointer());
    const auto m = ptr->As<Module>();
    return filter(m);
  });
  if (m == std::end(modules_)) {
    const auto home = GetHomeEnvVar();
    if (!home)
      return nullptr;
    const auto new_module = Module::LoadFrom(fmt::format("{}/lib/{}", (*home.value()), name));
    LOG_IF(FATAL, !new_module) << "failed to create new module from: " << name;
    LOG_IF(ERROR, !GetRuntime()->GetInitScope()->Add(new_module->GetScope())) << "failed to import the _kernel Module.";
    if (new_module->HasInit())
      LOG_IF(FATAL, !new_module->Init(GetRuntime())) << "failed to initialize the _kernel Module: " << new_module;
    return new_module;
  }
  return (*m)->As<Module>();
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

auto Module::VisitPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  {
    auto name_ptr = name_->raw_ptr();
    if (!vis->Visit(&name_ptr))
      return false;
    name_ = name_ptr->As<String>();
  }
  DLOG(INFO) << "visiting: " << scope_->ToString();
  LOG_IF(FATAL, !scope_->VisitLocalPointers([vis](Pointer** ptr) {
    return vis->Visit(ptr);
  })) << "failed to visit pointers in scope.";
  for (auto& ns : namespaces_) {
    auto ns_ptr = ns->raw_ptr();
    if (!vis->Visit(&ns_ptr))
      return false;
    ns = ns_ptr->As<Namespace>();
  }
  return true;
}

Field* Module::kFieldInitialized = nullptr;
auto Module::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  const auto cls = Class::New(Object::GetClass(), "Module");
  ASSERT(cls);
  kFieldInitialized = cls->AddField("initialized");
  ASSERT(kFieldInitialized);
  return cls;
}

auto Module::New(const ObjectList& args) -> Module* {
  NOT_IMPLEMENTED(FATAL);
}

auto Module::VisitModules(const std::function<bool(Module*)>& vis) -> bool {
  for (const auto& ptr : modules_) {
    ASSERT(ptr && ptr->GetObjectPointer());
    if (!vis(ptr->As<Module>()))
      return false;
  }
  return true;
}

auto Module::VisitModulePointers(const std::function<bool(Pointer**)>& vis) -> bool {
  for (auto& ptr : modules_) {
    ASSERT(ptr && ptr->GetObjectPointer());
    if (!vis(&ptr))
      return false;
  }
  return true;
}
}  // namespace gel