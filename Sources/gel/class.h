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
class PointerPointerVisitor;
using ClassList = std::vector<Class*>;

class ClassVisitor {
  DEFINE_NON_COPYABLE_TYPE(ClassVisitor);

 protected:
  ClassVisitor() = default;

 public:
  virtual ~ClassVisitor() = default;
  virtual auto Visit(Class* cls) -> bool = 0;
};
DECLARE_VISITOR_WRAPPER(Class, Class*);

class Class : public Object {
  friend class Long;
  friend class Object;
  friend class Parser;

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
    kEventEmitterClassId,
    kTotalNumberOfInternalClassIds,
  };

 private:
  ClassId id_;
  Class* parent_;
  String* name_;
  Array<Field*>* fields_;
  std::vector<Procedure*> funcs_{};

 protected:
  explicit Class(ClassId id, Class* parent, String* name);

  inline void SetParent(Class* cls) {
    ASSERT(cls);
    parent_ = cls;
  }

  inline void SetName(String* name) {
    ASSERT(name);
    name_ = name;
  }

  void AddChild(Object* rhs) override;
  auto CreateSymbol(const std::string& name) -> Symbol*;
  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

  auto FindOrCreateNativeProcedure(Symbol* symbol) -> NativeProcedure*;

 public:
  ~Class() override = default;

  auto GetClassId() const -> ClassId {
    return id_;
  }

  inline auto IsPrimitive() const -> bool {
    return GetClassId() >= 0 && GetClassId() <= kTotalNumberOfInternalClassIds;
  }

  void AddFunction(Procedure* func) {
    ASSERT(func);
    funcs_.push_back(func);
  }

  auto GetFields() const -> Array<Field*>* {
    return fields_;
  }

  void Add(Field* field);
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

  auto GetNumberOfFields() const -> uint64_t;
  auto GetFieldAt(const uint64_t idx) const -> Field*;

  auto GetNumberOfProcedures() const -> uint64_t;
  auto GetProcedureAt(const uint64_t idx) const -> Procedure*;

  auto NewInstance(const ObjectList& args) -> Object*;
  auto GetAllocationSize() const -> uword;
  auto IsInstanceOf(Class* rhs) const -> bool;
  auto HasFunction(Symbol* symbol, const bool recursive = true) const -> bool;
  auto FindFunction(const std::string& name, const bool recursive = true) const -> Procedure*;
  auto FindFunction(Symbol* symbol, const bool recursive = true) const -> Procedure*;
  auto FindField(Symbol* symbol, const bool recursive = true) const -> Field*;
  auto FindField(const std::string& name, const bool recursive = true) const -> Field*;
  DECLARE_TYPE(Class);

 private:
  static void Init();
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

  static auto VisitAllClasses(ClassVisitor* vis) -> bool;
  static auto VisitAllClassPointers(PointerVisitor* vis) -> bool;
  static auto VisitAllClassPointerPointers(PointerPointerVisitor* vis) -> bool;

#ifdef GEL_DEBUG
  static inline auto PrintAllClasses() -> bool {
    ClassVisitorWrapper vis([](Class* cls) {
      ASSERT(cls);
      DLOG(INFO) << cls->ToString() << "  ;;  " << (*cls->raw_ptr());
      return true;
    });
    DLOG(INFO) << "classes:";
    return VisitAllClasses(&vis);
  }
#endif  // GEL_DEBUG
};

class Field : public Object {
  friend class Class;

 public:
  using Predicate = std::function<bool(Field*)>;

  static auto IsNamed(const std::string& name) -> Predicate;

 private:
  Class* owner_;
  String* name_;
  uword offset_ = 0;

 protected:
  Field(Class* owner, String* name) :
    Object(),
    owner_(owner),
    name_(name) {
    ASSERT(owner_);
    ASSERT(name_);
  }

  void SetOffset(const uword offset) {
    offset_ = offset;
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~Field() override = default;

  auto GetOwner() const -> Class* {
    return owner_;
  }

  auto GetName() const -> String* {
    return name_;
  }

  auto GetOffset() const -> uword {
    return offset_;
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
};

}  // namespace gel

#endif  // GEL_CLASS_H
