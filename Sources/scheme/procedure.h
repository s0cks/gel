#ifndef SCM_PROCEDURE_H
#define SCM_PROCEDURE_H

#include "scheme/common.h"
#include "scheme/local_scope.h"
#include "scheme/object.h"

namespace scm {
class Procedure : public Object {
  friend class Runtime;
  friend class Interpreter;
  DEFINE_NON_COPYABLE_TYPE(Procedure);

 protected:
  Procedure() = default;

  virtual void Apply() {
    // do nothing
  }

 public:
  ~Procedure() override = default;

  auto Equals(Object* rhs) const -> bool override {
    return rhs && rhs->IsProcedure();
  }

  auto AsProcedure() -> Procedure* override {
    return this;
  }

  virtual auto IsNative() const -> bool {
    return false;
  }

  auto GetType() const -> Class* override {
    return GetClass();
  }

 private:
  static Class* kClass;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

 public:
  static void Init();

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};
}  // namespace scm

#endif  // SCM_PROCEDURE_H
