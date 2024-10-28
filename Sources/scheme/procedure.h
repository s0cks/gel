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

  virtual auto Apply(Runtime* state) const -> bool = 0;
};

class NativeProcedure : public Procedure {
  DEFINE_NON_COPYABLE_TYPE(NativeProcedure);

 private:
  Symbol* symbol_;

 protected:
  explicit NativeProcedure(Symbol* symbol) :
    Procedure(),
    symbol_(symbol) {
    ASSERT(symbol_);
  }

 public:
  ~NativeProcedure() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto IsNative() const -> bool override {
    return true;
  }

  auto ToString() const -> std::string override;

  auto GetTypename() const -> const char* override {
    return "NativeProcedure";
  }
};

#define _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym) \
  DEFINE_NON_COPYABLE_TYPE(Name);                \
                                                 \
 public:                                         \
  Name() :                                       \
    NativeProcedure(Symbol::New(Sym)) {}         \
  ~Name() override = default;                    \
  auto Apply(Runtime* state) const -> bool override;

#define DEFINE_NATIVE_PROCEDURE_TYPE(Name) _DEFINE_NATIVE_PROCEDURE_TYPE(Name, #Name)

#define _DECLARE_NATIVE_PROCEDURE(Name, Sym)  \
  class Name : public NativeProcedure {       \
    _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym); \
  };

#define DECLARE_NATIVE_PROCEDURE(Name)  \
  class Name : public NativeProcedure { \
    DEFINE_NATIVE_PROCEDURE_TYPE(Name); \
  };

#define NATIVE_PROCEDURE_F(Name) auto Name::Apply(Runtime* state) const -> bool

class Macro : public Procedure {
 public:
  DECLARE_TYPE(Macro);
};
}  // namespace scm

#endif  // SCM_PROCEDURE_H
