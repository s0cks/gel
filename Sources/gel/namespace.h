#ifndef GEL_NAMESPACE_H
#define GEL_NAMESPACE_H

#include "gel/object.h"

namespace gel {
class Namespace : public Object {
 private:
  String* name_;
  LocalScope* scope_;

 protected:
  explicit Namespace(String* name, LocalScope* scope) :
    Object(),
    name_(name),
    scope_(scope) {
    ASSERT(name_);
    ASSERT(scope_);
  }

 public:
  ~Namespace() override = default;

  auto GetName() const -> String* {
    return name_;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  DECLARE_TYPE(Namespace);

 public:
  static inline auto New(String* name, LocalScope* scope) -> Namespace* {
    ASSERT(name);
    return new Namespace(name, scope);
  }
};
}  // namespace gel

#endif  // GEL_NAMESPACE_H
