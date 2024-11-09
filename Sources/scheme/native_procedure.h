#ifndef SCM_NATIVE_PROCEDURE_H
#define SCM_NATIVE_PROCEDURE_H

#include "scheme/error.h"
#include "scheme/procedure.h"

namespace scm {
class NativeProcedure : public Procedure {
  friend class Interpreter;
  friend class Runtime;

 private:
  Symbol* symbol_;

 protected:
  explicit NativeProcedure(Symbol* symbol) :
    Procedure(),
    symbol_(symbol) {
    ASSERT(symbol_);
  }

  auto ReturnValue(Object* rhs) const -> bool;
  virtual auto ApplyProcedure(const std::vector<Object*>& args) const -> bool = 0;
  virtual auto Apply(const std::vector<Object*>& args) const -> bool;

  inline auto ThrowError(const std::string& message) const -> bool {
    return ReturnValue(Error::New(message));
  }

 public:
  ~NativeProcedure() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto IsNative() const -> bool override {
    return true;
  }

  auto GetType() const -> Class* override {
    return GetClass();
  }

  DECLARE_TYPE(NativeProcedure);

 private:
  static Class* kClass;

 public:
  static void Init();

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};

#define _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym)                                \
  DEFINE_NON_COPYABLE_TYPE(Name);                                               \
                                                                                \
 protected:                                                                     \
  auto ApplyProcedure(const std::vector<Object*>& args) const -> bool override; \
                                                                                \
 public:                                                                        \
  Name() :                                                                      \
    NativeProcedure(Symbol::New(Sym)) {}                                        \
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

#define NATIVE_PROCEDURE_F(Name) auto Name::ApplyProcedure(const std::vector<Object*>& args) const -> bool
}  // namespace scm

#endif  // SCM_NATIVE_PROCEDURE_H
