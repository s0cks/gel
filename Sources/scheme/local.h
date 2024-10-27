#ifndef SCM_LOCAL_H
#define SCM_LOCAL_H

#include <ostream>
#include <string>
#include <utility>

#include "scheme/common.h"

namespace scm {
class Type;
class LocalScope;
class LocalVariable {
  friend class LocalScope;
  DEFINE_NON_COPYABLE_TYPE(LocalVariable);

 private:
  LocalScope* owner_;
  uint64_t index_;
  std::string name_;
  Type* value_;

  LocalVariable(LocalScope* owner, uint64_t index, std::string name, Type* value = nullptr) :
    owner_(owner),
    index_(index),
    name_(std::move(name)),
    value_(value) {}

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

  void SetConstantValue(Type* value) {
    ASSERT(value);
    value_ = value;
  }

 public:
  ~LocalVariable() = default;

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

  auto GetValue() const -> Type* {
    return value_;
  }

  auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  friend auto operator<<(std::ostream& stream, const LocalVariable& rhs) -> std::ostream& {
    stream << "LocalVariable(";
    stream << "owner=" << rhs.GetOwner() << ", ";
    stream << "index=" << rhs.GetIndex() << ", ";
    stream << "name=" << rhs.GetName() << ", ";
    if (rhs.HasValue())
      stream << "value=" << rhs.GetValue();
    stream << ")";
    return stream;
  }
};
}  // namespace scm

#endif  // SCM_LOCAL_H
