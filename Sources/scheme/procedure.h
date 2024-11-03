#ifndef SCM_PROCEDURE_H
#define SCM_PROCEDURE_H

#include "scheme/common.h"
#include "scheme/local_scope.h"
#include "scheme/type.h"

namespace scm {
class State;
class Runtime;
class Procedure : public Type {
  DEFINE_NON_COPYABLE_TYPE(Procedure);

 protected:
  Procedure() = default;

 public:
  ~Procedure() override = default;

  auto Equals(Type* rhs) const -> bool override {
    return rhs && rhs->IsProcedure();
  }

  auto AsProcedure() -> Procedure* override {
    return this;
  }

  virtual auto IsNative() const -> bool {
    return false;
  }

  virtual auto Apply(Runtime* state) -> bool = 0;
};

static inline auto IsProcedure(Type* rhs) -> bool {
  return rhs && rhs->IsProcedure();
}
}  // namespace scm

#endif  // SCM_PROCEDURE_H
