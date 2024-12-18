#include "gel/module.h"

#include "gel/common.h"
#include "gel/parser.h"
#include "gel/to_string_helper.h"

namespace gel {
std::vector<Pointer*> modules_{};

static inline auto Register(Module* m) -> Module* {
  ASSERT(m);
  modules_.push_back(m->raw_ptr());
  return m;
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

auto Module::New(String* name, LocalScope* scope) -> Module* {
  ASSERT(name);
  ASSERT(scope);
  return Register(new Module(name, scope));
}

auto Module::LoadFrom(const std::filesystem::path& abs_path) -> Module* {
  DVLOG(100) << "loading Module from: " << abs_path << "....";
  return Parser::ParseModuleFrom(abs_path);
}

auto Module::ToString() const -> std::string {
  ToStringHelper<Module> helper;
  helper.AddField("name", GetName());
  return helper;
}

auto Module::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsModule())
    return false;
  const auto other = rhs->AsModule();
  ASSERT(other);
  return GetName()->Equals(other->GetName());
}

auto Module::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Module");
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