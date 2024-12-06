#include "gel/common.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/to_string_helper.h"

namespace gel {
PointerList classes_;

static inline auto Register(Class* cls) -> Class* {
  ASSERT(cls);
  classes_.push_back(cls->raw_ptr());
  return cls;
}

auto Class::New(Class* parent, String* name) -> Class* {
  ASSERT(parent);
  ASSERT(name);
  return Register(new Class(parent, name));
}

auto Class::New(String* name) -> Class* {
  ASSERT(name);
  return Register(new Class(nullptr, name));
}

auto Class::New(const std::string& name) -> Class* {
  ASSERT(!name.empty());
  return New(String::New(name));
}

auto Class::New(Class* parent, const std::string& name) -> Class* {
  ASSERT(parent);
  ASSERT(!name.empty());
  return New(parent, String::New(name));
}

auto Class::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Class::New(const ObjectList& args) -> Class* {
  NOT_IMPLEMENTED(FATAL);
}

auto Class::ToString() const -> std::string {
  ToStringHelper<Class> helper;
  helper.AddField("name", GetName()->Get());
  if (HasParent())
    helper.AddField("parent", GetParent()->GetName()->Get());
  return helper;
}

auto Class::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Class::IsInstanceOf(Class* rhs) const -> bool {
  ASSERT(rhs);
  auto cls = this;
  while (cls) {
    if (cls->Equals(rhs))
      return true;
    cls = cls->GetParent();
  }
  return false;
}

auto Class::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsClass())
    return false;
  const auto other = rhs->AsClass();
  ASSERT(other);
  return GetName()->Equals(other->GetName());
}

auto Class::FindClass(const std::string& name) -> Class* {
  for (const auto& ptr : classes_) {
    ASSERT(ptr && ptr->GetObjectPointer());
    const auto cls = ptr->As<Class>();
    if (cls->GetName()->Get() == name)
      return cls;
  }
  return nullptr;
}

auto Class::FindClass(String* name) -> Class* {
  return FindClass(name->Get());
}

auto Class::FindClass(Symbol* name) -> Class* {
  return FindClass(name->Get());
}

template <typename V>
static inline auto IterateClasses(const V& values, const std::function<bool(Class*)>& vis) -> bool {
  for (const Pointer* ptr : values) {
    ASSERT(ptr && ptr->GetObjectPointer());
    if (!vis(ptr->As<Class>()))
      return false;
  }
  return true;
}

auto Class::VisitClasses(const std::function<bool(Class*)>& vis, const bool reverse) -> bool {
  return reverse ? IterateClasses(std::ranges::reverse_view(classes_), vis) : IterateClasses(classes_, vis);
}

auto Class::VisitClassPointers(const std::function<bool(Pointer**)>& vis) -> bool {
  for (auto& ptr : classes_) {
    ASSERT(ptr && ptr->GetObjectPointer());
    if (!vis(&ptr))
      return false;
  }
  return true;
}
}  // namespace gel