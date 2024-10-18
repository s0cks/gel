#ifndef SCM_PROCEDURE_H
#define SCM_PROCEDURE_H

#include "scheme/environment.h"
#include "scheme/type.h"

namespace scm {
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

  virtual auto Apply(Runtime*, Datum* rhs) const -> Datum* = 0;
};

class Lambda : public Procedure {
 public:
  DECLARE_TYPE(Lambda);
  auto Apply(Runtime*, Datum* rhs) const -> Datum* override;
};

class Macro : public Procedure {
 public:
  DECLARE_TYPE(Macro);
};
}  // namespace scm

#endif  // SCM_PROCEDURE_H