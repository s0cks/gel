#ifndef SCM_NATIVE_PROCEDURE_H
#define SCM_NATIVE_PROCEDURE_H

#include "scheme/procedure.h"

namespace scm {
class NativeProcedure : public Procedure {
  friend class Interpreter;

 private:
  Symbol* symbol_;

 protected:
  explicit NativeProcedure(Symbol* symbol) :
    Procedure(),
    symbol_(symbol) {
    ASSERT(symbol_);
  }

  virtual auto ApplyProcedure(Runtime* runtime, const std::vector<Type*>& args) const -> bool = 0;

 public:
  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto IsNative() const -> bool override {
    return true;
  }

  auto Apply(Runtime* runtime) const -> bool override;
  DECLARE_TYPE(NativeProcedure);
};

static inline auto IsNativeProcedure(Type* rhs) -> bool {
  return rhs && rhs->IsProcedure() && rhs->AsProcedure()->IsNative();
}

#define _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym)                                              \
 protected:                                                                                   \
  auto ApplyProcedure(Runtime* state, const std::vector<Type*>& args) const -> bool override; \
                                                                                              \
 public:                                                                                      \
  Name() :                                                                                    \
    NativeProcedure(Symbol::New(Sym)) {}                                                      \
  ~Name() override = default;

#define DEFINE_NATIVE_PROCEDURE_TYPE(Name) _DEFINE_NATIVE_PROCEDURE_TYPE(Name, #Name)

#define _DECLARE_NATIVE_PROCEDURE(Name, Sym)  \
  class Name : public NativeProcedure {       \
    _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym); \
  };

#define DECLARE_NATIVE_PROCEDURE(Name)  \
  class Name : public NativeProcedure { \
    DEFINE_NATIVE_PROCEDURE_TYPE(Name); \
  };

#define NATIVE_PROCEDURE_F(Name) auto Name::ApplyProcedure(Runtime* state, const std::vector<Type*>& args) const -> bool
}  // namespace scm

#endif  // SCM_NATIVE_PROCEDURE_H