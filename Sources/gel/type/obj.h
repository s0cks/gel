#ifndef GEL_OBJ_H
#define GEL_OBJ_H

#include <unordered_map>

#include "gel/type/value.h"

namespace gel {
class Obj : public Value {
  struct PropertyEq {
    inline auto operator()();
  };

 private:
  Obj* parent_;
  std::unordered_map<std::string, Value*> properties_{};

 public:
  explicit Obj(Obj* parent = nullptr) :
    Value(),
    parent_(parent) {}
  ~Obj() override = default;

  auto GetParent() const -> Obj* {
    return parent_;
  }

  inline auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  void SetParent(Obj* rhs) {
    ASSERT(rhs);
    parent_ = rhs;
  }

  inline void RemoveParent() {
    parent_ = nullptr;
  }

  auto PutProperty(Str* name, Value* value) -> bool;
  auto GetProperty(Str* name) const -> Value*;

  inline auto HasProperty(Str* name) const -> bool {
    return GetProperty(name) != nullptr;
  }

  DECLARE_VALUE_TYPE(Obj);

 public:
  inline friend auto operator<<(std::ostream& stream, const Obj& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }
};
}  // namespace gel

#endif  // GEL_OBJ_H
