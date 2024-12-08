#include "gel/common.h"
#ifndef GEL_OBJECT_H
#error "Please #include <gel/object.h> instead."
#endif  // GEL_OBJECT_H

#ifndef GEL_CLASS_H
#define GEL_CLASS_H

#include <vector>

#include "gel/object.h"

namespace gel {
class Class;
using ClassList = std::vector<Class*>;

class Class : public Object {
  friend class Object;
  using FieldList = std::vector<Field*>;

 private:
  Class* parent_;
  String* name_;
  FieldList fields_{};  // TODO: @s0cks need to use Array<Field*> to enable GC

 protected:
  explicit Class(Class* parent, String* name) :
    Object(),
    parent_(parent),
    name_(name) {
    ASSERT(name_);
  }

  inline void Add(Field* field) {
    ASSERT(field);
    fields_.push_back(field);
  }

  auto AddField(const std::string& name, Class* type, Instance* value = nullptr) -> Field*;

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~Class() override = default;

  auto GetType() const -> Class* override {
    return GetClass();
  }

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

  auto GetFields() const -> const FieldList& {
    return fields_;
  }

  auto GetAllocationSize() const -> uword;
  auto IsInstanceOf(Class* rhs) const -> bool;
  DECLARE_TYPE(Class);

 private:
  static auto New(String* name) -> Class*;
  static auto New(const std::string& name) -> Class*;

 public:
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
  std::string name_;
  Class* type_;
  bool constant_ = false;
  union {
    Instance* value_;
    uword offset_;
  };

 protected:
  Field(Class* owner, const std::string& name, Class* type) :
    owner_(owner),
    name_(name),
    type_(type),
    offset_(0) {}
  Field(Class* owner, const std::string& name, Class* type, Instance* value) :
    owner_(owner),
    name_(name),
    type_(type),
    value_(value) {
    ASSERT(value_);  // NOLINT(cppcoreguidelines-pro-type-union-access)
    SetConstant();
  }

  inline void SetConstant(const bool rhs = true) {
    constant_ = rhs;
  }

 public:
  ~Field() override = default;

  auto GetType() const -> Class* override {
    return GetClass();
  }

  auto GetFieldType() const -> Class* {
    return type_;
  }

  auto GetOwner() const -> Class* {
    return owner_;
  }

  inline auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  auto GetName() const -> const std::string& {
    return name_;
  }

  inline auto IsConstant() const -> bool {
    return constant_;
  }

  auto GetOffset() const -> uword {
    ASSERT(!IsConstant());
    return offset_;  // NOLINT(cppcoreguidelines-pro-type-union-access)
  }

  auto GetValue() const -> Instance* {
    ASSERT(IsConstant());
    return value_;  // NOLINT(cppcoreguidelines-pro-type-union-access)
  }

  DECLARE_TYPE(Field);

 private:
  static inline auto New(Class* owner, const std::string& name, Class* type) -> Field* {
    ASSERT(owner);
    ASSERT(type);
    return new Field(owner, name, type);
  }

  static inline auto New(Class* owner, const std::string& name, Class* type, Instance* value) -> Field* {
    ASSERT(owner);
    ASSERT(type);
    return new Field(owner, name, type, value);
  }
};
}  // namespace gel

#endif  // GEL_CLASS_H
