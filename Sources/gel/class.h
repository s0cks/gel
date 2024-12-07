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

class Class : public Datum {
  friend class Object;

 private:
  Class* parent_;
  String* name_;

 protected:
  explicit Class(Class* parent, String* name) :
    Datum(),
    parent_(parent),
    name_(name) {
    ASSERT(name_);
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~Class() override = default;

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
}  // namespace gel

#endif  // GEL_CLASS_H
