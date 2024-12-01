#ifndef GEL_LOCAL_H
#define GEL_LOCAL_H

#include <ostream>
#include <string>
#include <utility>

#include "gel/object.h"

namespace gel {
class LocalVariable;
class PointerPointerVisitor;
class LocalVariableVisitor {
  DEFINE_NON_COPYABLE_TYPE(LocalVariableVisitor);

 protected:
  LocalVariableVisitor() = default;

 public:
  virtual ~LocalVariableVisitor() = default;
  virtual auto VisitLocal(LocalVariable* local) -> bool = 0;
};

class LocalScope;
class LocalVariable {
  friend class LocalScope;
  DEFINE_NON_COPYABLE_TYPE(LocalVariable);

 private:
  LocalScope* owner_;
  uint64_t index_;
  std::string name_;
  Pointer* value_ = nullptr;

  LocalVariable(LocalScope* owner, uint64_t index, std::string name, Object* value = nullptr) :
    owner_(owner),
    index_(index),
    name_(std::move(name)) {
    if (value)
      SetValue(value);
  }

  void SetOwner(LocalScope* scope) {
    ASSERT(scope);
    owner_ = scope;
  }

  void SetIndex(const uint64_t index) {
    index_ = index;
  }

  void SetName(const std::string& name) {
    ASSERT(!name.empty());
    name_ = name;
  }

  auto Accept(PointerVisitor* vis) -> bool;
  auto Accept(PointerPointerVisitor* vis) -> bool;

  auto Accept(const std::function<bool(Pointer**)>& vis) -> bool {
    return vis(&value_);
  }

 public:
  ~LocalVariable() = default;

  auto ptr() const -> Pointer* {
    return value_;
  }

  auto GetOwner() const -> LocalScope* {
    return owner_;
  }

  auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  auto GetIndex() const -> uint64_t {
    return index_;
  }

  auto GetName() const -> const std::string& {
    return name_;
  }

  auto GetValue() const -> Object*;
  void SetValue(Object* rhs);

  auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

#define DEFINE_TYPE_CHECK(Name)                  \
  inline auto Is##Name() const->bool {           \
    return HasValue() && GetValue()->Is##Name(); \
  }
  FOR_EACH_TYPE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK

  auto IsGlobal() const -> bool;

  friend auto operator<<(std::ostream& stream, const LocalVariable& rhs) -> std::ostream& {
    stream << "LocalVariable(";
    if (rhs.HasOwner())
      stream << "owner=" << rhs.GetOwner() << ", ";
    stream << "index=" << rhs.GetIndex() << ", ";
    stream << "name=" << rhs.GetName();
    if (rhs.HasValue())
      stream << ", value=" << rhs.GetValue()->ToString();
    stream << ")";
    return stream;
  }

 public:
  static inline auto New(LocalScope* owner, const uint64_t index, const std::string& name, Object* value = nullptr)
      -> LocalVariable* {
    ASSERT(owner);
    ASSERT(index >= 0);
    ASSERT(!name.empty());
    return new LocalVariable(owner, index, name, value);
  }

  static auto New(LocalScope* owner, const std::string& name, Object* value = nullptr) -> LocalVariable*;
  static auto New(LocalScope* owner, const Symbol* symbol, Object* value = nullptr) -> LocalVariable*;
};
}  // namespace gel

#endif  // GEL_LOCAL_H
