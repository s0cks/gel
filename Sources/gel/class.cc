#include "gel/array.h"
#include "gel/buffer.h"
#include "gel/common.h"
#include "gel/event_emitter.h"
#include "gel/event_loop.h"
#include "gel/macro.h"
#include "gel/module.h"
#include "gel/namespace.h"
#include "gel/native_procedure.h"
#include "gel/natives.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/script.h"
#include "gel/to_string_helper.h"

namespace gel {
static Array<Class*>* classes_ = nullptr;

static inline auto Register(Class* cls) -> Class* {
  ASSERT(cls);
  classes_->Push(cls);
  return cls;
}

Class::Class(ClassId id, Class* parent, String* name) :
  Object(),
  id_(id),
  parent_(parent),
  name_(name),
  fields_(Array<Field*>::New()) {
  ASSERT(name_);
  ASSERT(fields_);
}

auto Class::New(const ClassId id, Class* parent, String* name) -> Class* {
  ASSERT(name);
  const auto cls = new Class(id, parent, name);
  ASSERT(cls);
  return Register(cls);
}

auto Class::New(const ClassId id, Class* parent, const std::string& name) -> Class* {
  ASSERT(parent);
  return New(id, parent, String::New(name));
}

auto Class::New(Class* parent, String* name) -> Class* {
  ASSERT(parent);
  ASSERT(name);
  return New(classes_->GetLength() + 1, parent, name);
}

auto Class::New(const ClassId id, String* name) -> Class* {
  ASSERT(name);
  return Class::New(id, nullptr, name);
}

auto Class::New(const ClassId id, const std::string& name) -> Class* {
  ASSERT(!name.empty());
  return New(id, String::New(name));
}

auto Class::New(Class* parent, const std::string& name) -> Class* {
  ASSERT(parent);
  ASSERT(!name.empty());
  return New(parent, String::New(name));
}

auto Class::CreateClass() -> Class* {
  const auto cls = Class::New(Object::GetClass(), kClassName);
  ASSERT(cls);
  return cls;
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

auto Class::GetAllocationSize() const -> uword {
  if (Equals(Class::kClass)) {
    const auto cls = Class::GetClass();
    ASSERT(cls);
    uword total_size = sizeof(Class);
    for (auto idx = 0; idx < cls->GetNumberOfFields(); idx++) {
      const auto field = cls->GetFieldAt(idx);
      ASSERT(field);
      field->SetOffset(total_size);
      total_size += sizeof(uword);
    }
    return total_size;
  } else if (Equals(Field::kClass))
    return sizeof(Field);
  else if (Equals(String::kClass))
    return sizeof(String);
  else if (Equals(Module::kClass)) {
    const auto cls = Module::GetClass();
    ASSERT(cls);
    uword total_size = sizeof(Module);
    for (auto idx = 0; idx < cls->GetNumberOfFields(); idx++) {
      const auto field = cls->GetFieldAt(idx);
      ASSERT(field);
      field->SetOffset(total_size);
      total_size += sizeof(uword);
    }
    return total_size;
  }
  return 0;
}

auto Class::AddField(const std::string& name) -> Field* {
  ASSERT(!name.empty());
  const auto field = Field::New(this, String::New(name));
  ASSERT(field);
  Add(field);
  return field;
}

auto Class::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (HasParent()) {
    if (!vis->Visit(GetParent()->raw_ptr()))
      return false;
  }
  if (!vis->Visit(GetName()->raw_ptr()))
    return false;
  if (!vis->Visit(GetFields()->raw_ptr()))
    return false;
  for (const auto& func : funcs_) {
    ASSERT(func);
    if (!vis->Visit(func->raw_ptr()))
      return false;
  }
  return true;
}

auto Class::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!VisitPointerPointer(vis, &parent_))
    return false;
  if (!VisitPointerPointer(vis, &name_))
    return false;
  if (!VisitPointerPointer(vis, &fields_))
    return false;
  return true;
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

auto Class::HashCode() const -> uword {
  uword hash = 0;
  CombineHash(hash, GetName()->Get());
  return hash;
}

static inline auto IsNamed(const std::string& name) -> std::function<bool(Class*)> {
  ASSERT(!name.empty());
  return [name](Class* cls) {
    return cls && cls->GetName()->Equals(name);
  };
}

auto Class::FindClass(const std::string& name) -> Class* {
  return classes_->FindIf(IsNamed(name));
}

auto Class::FindClass(String* name) -> Class* {
  return FindClass(name->Get());
}

auto Class::FindClass(Symbol* name) -> Class* {
  return FindClass(name->GetSymbolName());
}

auto Class::GetNumberOfFields() const -> uint64_t {
  return fields_->GetLength();
}

auto Class::GetFieldAt(const uint64_t idx) const -> Field* {
  ASSERT(idx >= 0 && idx <= GetNumberOfFields());
  return fields_->Get(idx);
}

auto Class::NewInstance(const ObjectList& args) -> Object* {
  // clang-format off
  if(Equals(Object::GetClass()))
    LOG(FATAL) << "cannot create a new instance of Object.";
#define INVOKE_NEW(Name)              \
  else if(Equals(Name::GetClass()))   \
    return Name::New(args); \
  // clang-format on
  FOR_EACH_TYPE(INVOKE_NEW)
#undef INVOKE_NEW
  LOG(FATAL) << "cannot create a new instance of " << ToString();
  return nullptr;
}

auto Class::FindFunction(const std::string& name, const bool recursive) const -> Procedure* {
  for (const auto& func : funcs_) {
    if (func->GetSymbol()->GetSymbolName() == name)
      return func;
  }
  if (recursive && HasParent()) {
    auto cls = GetParent();
    do {
      const auto func = cls->FindFunction(name, false);
      if (func)
        return func;
      cls = cls->GetParent();
    } while (cls);
  }
  return nullptr;
}

void Class::Add(Field* field) {
  ASSERT(field);
  fields_->Push(field);
}

auto Field::IsNamed(const std::string& name) -> Field::Predicate {
  ASSERT(!name.empty());
  return [&name](Field* field) {
    return field && field->GetName()->Equals(name);
  };
}

auto Class::FindField(Symbol* symbol, const bool recursive) const -> Field* {
  Class const* cls = this;
  do {
    const auto field = cls->GetFields()->FindIf(Field::IsNamed(symbol->GetSymbolName()));
    if (field)
      return field;
    if (!recursive)
      break;
    cls = cls->GetParent();
  } while (cls);
  DLOG(WARNING) << "failed to find field w/ symbol: " << symbol;
  return nullptr;
}

auto Class::FindFunction(Symbol* symbol, const bool recursive) const -> Procedure* {
  for (const auto& func : funcs_) {
    if (func->GetSymbol()->Equals(symbol))
      return func;
  }
  if (recursive && HasParent()) {
    auto cls = GetParent();
    do {
      const auto func = cls->FindFunction(symbol, false);
      if (func)
        return func;
      cls = cls->GetParent();
    } while (cls);
  }
  return nullptr;
}

auto Class::HasFunction(Symbol* symbol, const bool recursive) const -> bool {
  for (const auto& func : funcs_) {
    if (func->GetSymbol()->Equals(symbol))
      return true;
  }
  if (!recursive || !HasParent())
    return false;
  auto cls = GetParent();
  do {
    if (cls->HasFunction(symbol, false))
      return true;
    cls = cls->GetParent();
  } while (cls);
  return false;
}

auto Class::VisitAllClasses(ClassVisitor* vis) -> bool {
  ASSERT(vis);
  for (auto idx = 0; idx < classes_->GetLength(); idx++) {
    const auto cls = classes_->Get(idx);
    ASSERT(cls);
    if (!vis->Visit(cls))
      return false;
  }
  return true;
}

auto Class::VisitAllClassPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  return classes_->VisitPointers(vis);
}

auto Class::VisitAllClassPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!classes_->VisitPointerPointers(vis))
    return false;
  if (!VisitPointerPointer(vis, &classes_))
    return false;
#define VISIT_CLASS_POINTER_POINTER(Name)   \
  if (!Name::VisitClassPointerPointer(vis)) \
    return false;
  VISIT_CLASS_POINTER_POINTER(Object);
  FOR_EACH_TYPE(VISIT_CLASS_POINTER_POINTER)
#undef VISIT_CLASS_POINTER_POINTER
  return true;
}

auto Field::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Field");
}

auto Field::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!VisitPointerPointer(vis, &owner_))
    return false;
  if (!VisitPointerPointer(vis, &name_))
    return false;
  return true;
}

auto Field::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!vis->Visit(GetOwner()->raw_ptr()))
    return false;
  if (!vis->Visit(GetName()->raw_ptr()))
    return false;
  return true;
}

auto Field::ToString() const -> std::string {
  ToStringHelper<Field> helper;
  helper.AddField("name", GetName());
  helper.AddField("owner", GetOwner());
  helper.AddField("offset", GetOffset());
  return helper;
}

auto Field::HashCode() const -> uword {
  uword hash = 0;
  CombineHash(hash, GetName()->HashCode());
  CombineHash(hash, GetOwner()->HashCode());
  return hash;
}

auto Field::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  return false;
}

auto Field::New(const ObjectList& args) -> Field* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return nullptr;
}

void Class::Init() {
  classes_ = Array<Class*>::New(Class::kTotalNumberOfInternalClassIds);
  ASSERT(classes_);
  using namespace proc;
  InitNative<get_classes>();
  InitNative<get_class>();
  InitNative<get_class_id>();
  InitNative<is_primitive_class>();
}

namespace proc {
NATIVE_PROCEDURE_F(get_classes) {
  ASSERT(args.empty());
  Object* result = Null();
  ClassVisitorWrapper vis([&result](Class* cls) {
    result = Cons(cls, result);
    return true;
  });
  LOG_IF(FATAL, !Class::VisitAllClasses(&vis)) << "failed to visit classes.";
  return Return(result);
}

NATIVE_PROCEDURE_F(get_class) {
  NativeArgument<0, Symbol> symbol(args);
  if (!symbol)
    return Throw(symbol.GetError());
  return Return(Class::FindClass(symbol));
}

NATIVE_PROCEDURE_F(get_class_id) {
  NativeArgument<0> clsOrSym(args);
  if (!clsOrSym)
    return Throw(clsOrSym);
  if (clsOrSym->IsClass()) {
    return ReturnLong(clsOrSym->AsClass()->GetClassId());
  } else if (clsOrSym->IsSymbol()) {
    const auto cls = Class::FindClass(clsOrSym->AsSymbol());
    if (!cls)
      return ReturnNull();
    return ReturnLong(cls->GetClassId());
  }
  return ReturnLong(clsOrSym->GetClass()->GetClassId());
}

NATIVE_PROCEDURE_F(is_primitive_class) {
  NativeArgument<0> clsOrSym(args);
  if (!clsOrSym)
    return Throw(clsOrSym);
  if (clsOrSym->IsClass()) {
    return ReturnBool(clsOrSym->AsClass()->IsPrimitive());
  } else if (clsOrSym->IsSymbol()) {
    const auto cls = Class::FindClass(clsOrSym->AsSymbol());
    if (!cls)
      return ReturnFalse();
    return ReturnBool(cls->IsPrimitive());
  }
  return ReturnFalse();
}
}  // namespace proc
}  // namespace gel