#ifndef SCM_VARIABLE_H
#define SCM_VARIABLE_H

#include <string>
#include <utility>

#include "scheme/type.h"

namespace scm {
class Variable {
  DEFINE_DEFAULT_COPYABLE_TYPE(Variable);

 private:
  std::string name_;
  Datum* value_;

 public:
  explicit Variable(std::string name, Datum* value = nullptr) :
    name_(std::move(name)),
    value_(value) {}
  ~Variable() = default;

  auto GetName() const -> const std::string& {
    return name_;
  }

  auto GetValue() const -> Datum* {
    return value_;
  }

  auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  auto IsConstant() const -> bool {
    return HasValue();
  }

  auto ToString() const -> std::string;

  auto operator==(const Variable& rhs) const -> bool {
    if (GetName() != rhs.GetName())
      return false;
    if (!HasValue() && !rhs.HasValue())
      return true;
    if ((HasValue() && !rhs.HasValue()) || (!HasValue() && rhs.HasValue()))
      return false;
    return GetValue()->Equals(rhs.GetValue());
  }

  auto operator!=(const Variable& rhs) const -> bool {
    return !operator==(rhs);
  }

  auto operator<(const Variable& rhs) const -> bool {
    return GetName() < rhs.GetName();
  }
};

using VariableList = std::vector<Variable>;

static inline auto operator<<(std::ostream& stream, Variable* rhs) -> std::ostream& {
  ASSERT(rhs);
  return stream << rhs->ToString();
}
}  // namespace scm

#endif  // SCM_VARIABLE_H
