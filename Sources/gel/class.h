#ifndef GEL_OBJECT_H
#error "Please #include <gel/object.h> instead."
#endif  // GEL_OBJECT_H

#ifndef GEL_CLASS_H
#define GEL_CLASS_H

#include <set>
#include <variant>
#include <vector>

#include "gel/common.h"
#include "gel/object.h"

namespace gel {
using ClassId = uword;
class Field;
class Class;
class Object;
class Symbol;
class String;
using ClassList = std::vector<Class*>;
class Class : public Object {
  friend class Long;
  friend class Object;

 public:
  enum ClassIds : ClassId {
    kInvalidClassId = 0,
    kObjectClassId,
    kClassClassId,
    kFieldClassId,
    kStringClassId,
    kSymbolClassId,
    kNamespaceClassId,
    kModuleClassId,
    kSeqClassId,
    kMapClassId,
    kProcedureClassId,
    kLambdaClassId,
    kNativeProcedureClassId,
    kBufferClassId,
    kScriptClassId,
    kBoolClassId,
    kNumberClassId,
    kLongClassId,
    kDoubleClassId,
    kPairClassId,
    kArrayClassId,
    kMacroClassId,
    kErrorClassId,
    kSetClassId,
    kExpressionClassId,
    kEventLoopClassId,
    kTimerClassId,
    kObservableClassId,
    kObserverClassId,
    kSubjectClassId,
    kReplaySubjectClassId,
    kPublishSubjectClassId,
    kTotalNumberOfInternalClassIds,
  };

 private:
  ClassId id_;
  Class* parent_;
  String* name_;
  std::vector<Procedure*> funcs_{};
  std::vector<Field*> fields_{};

 protected:
  explicit Class(ClassId id, Class* parent, String* name) :
    Object(),
    id_(id),
    parent_(parent),
    name_(name) {
    ASSERT(name_);
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~Class() override = default;

  auto GetClassId() const -> ClassId {
    return id_;
  }

  inline auto IsInternalClass() const -> bool {
    return GetClassId() >= 0 && GetClassId() <= kTotalNumberOfInternalClassIds;
  }

  void AddFunction(Procedure* func) {
    ASSERT(func);
    funcs_.push_back(func);
  }

  auto GetFields() const -> const std::vector<Field*>& {
    return fields_;
  }

  void Add(Field* field) {
    ASSERT(field);
    fields_.push_back(field);
  }

  auto AddField(const std::string& name) -> Field*;

  auto GetParent() const -> Class* {
    return parent_;
  }

  inline auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  auto GetName() const -> String* {
    return name_;
  }

  template <class T>
  inline auto Is() const -> bool {
    return Equals(T::GetClass());
  }

  template <class T>
  inline auto IsInstance() const -> bool {
    return IsInstanceOf(T::GetClass());
  }

  auto NewInstance(const ObjectList& args) -> Object*;
  auto GetAllocationSize() const -> uword;
  auto IsInstanceOf(Class* rhs) const -> bool;
  auto HasFunction(Symbol* symbol, const bool recursive = true) const -> bool;
  auto GetFunction(const std::string& name, const bool recursive = true) const -> Procedure*;
  auto GetFunction(Symbol* symbol, const bool recursive = true) const -> Procedure*;
  auto GetField(Symbol* symbol, const bool recursive = true) const -> Field*;
  DECLARE_TYPE(Class);

 private:
  static auto New(const ClassId id, String* name) -> Class*;
  static auto New(const ClassId id, const std::string& name) -> Class*;

 public:
  static auto GetTotalNumberOfClasses() -> uword;

  static auto New(const ClassId id, Class* parent, String* name) -> Class*;
  static auto New(const ClassId id, Class* parent, const std::string& name) -> Class*;
  static auto New(Class* parent, String* name) -> Class*;
  static auto New(Class* parent, const std::string& name) -> Class*;

  static auto FindClass(const std::string& name) -> Class*;
  static auto FindClass(String* name) -> Class*;
  static auto FindClass(Symbol* name) -> Class*;

  static auto VisitClasses(const std::function<bool(Class*)>& vis, const bool reverse = false) -> bool;
  static auto VisitClassPointers(const std::function<bool(Pointer**)>& vis) -> bool;
};

class Field : public Object {
  friend class Class;

 private:
  Class* owner_;
  String* name_;
  std::variant<uword, Object*> value_;

  Field(Class* owner, String* name) :
    Object(),
    owner_(owner),
    name_(name) {
    ASSERT(owner_);
    ASSERT(name_);
  }

  void SetValue(Object* rhs) {
    ASSERT(rhs);
    value_ = rhs;
  }

  void SetOffset(const uword offset) {
    value_ = offset;
  }

 public:
  ~Field() override = default;

  auto GetOwner() const -> Class* {
    return owner_;
  }

  auto GetName() const -> String* {
    return name_;
  }

  auto IsInstance() const -> bool {
    return std::holds_alternative<uword>(value_);
  }

  auto GetOffset() const -> uword {
    return std::get<uword>(value_);
  }

  auto IsStatic() const -> bool {
    return std::holds_alternative<Object*>(value_);
  }

  auto GetValue() const -> Object* {
    return std::get<Object*>(value_);
  }

  DECLARE_TYPE(Field);

 public:
  static inline auto New(Class* owner, String* name) -> Field* {
    ASSERT(owner);
    ASSERT(name);
    return new Field(owner, name);
  }

  static inline auto New(Class* owner, String* name, const uword offset) -> Field* {
    ASSERT(owner);
    ASSERT(name);
    const auto field = New(owner, name);
    ASSERT(field);
    field->SetOffset(offset);
    return field;
  }

  static inline auto New(Class* owner, String* name, Object* value) -> Field* {
    ASSERT(owner);
    ASSERT(name);
    ASSERT(value);
    const auto field = New(owner, name);
    ASSERT(field);
    field->SetValue(value);
    return field;
  }
};

}  // namespace gel

#endif  // GEL_CLASS_H
