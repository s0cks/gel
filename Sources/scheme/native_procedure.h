#ifndef SCM_NATIVE_PROCEDURE_H
#define SCM_NATIVE_PROCEDURE_H

#include "scheme/error.h"
#include "scheme/procedure.h"

namespace scm {
class Runtime;
class NativeProcedure : public Procedure {
  friend class Runtime;
  friend class Interpreter;

 private:
  Symbol* symbol_;

 protected:
  explicit NativeProcedure(Symbol* symbol) :
    Procedure(),
    symbol_(symbol) {
    ASSERT(symbol_);
  }

  auto Return(Object* rhs) const -> bool;
  virtual auto ApplyProcedure(const std::vector<Object*>& args) const -> bool = 0;
  virtual auto Apply(const std::vector<Object*>& args) const -> bool;

  template <class T, typename... Args>
  inline auto ReturnNew(Args... args) const -> bool {
    return Return(T::New(args...));
  }

  inline auto ThrowError(const std::string& message) const -> bool {
    return Return(Error::New(message));
  }

  inline auto DoNothing() const -> bool {
    return true;
  }

 public:
  ~NativeProcedure() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto IsNative() const -> bool override {
    return true;
  }

  DECLARE_TYPE(NativeProcedure);
};

#define _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym)                                \
  friend class scm::Runtime;                                                    \
  DEFINE_NON_COPYABLE_TYPE(Name);                                               \
                                                                                \
 protected:                                                                     \
  auto ApplyProcedure(const std::vector<Object*>& args) const -> bool override; \
                                                                                \
 public:                                                                        \
  Name() :                                                                      \
    NativeProcedure(kSymbol) {}                                                 \
  ~Name() override = default;                                                   \
                                                                                \
 private:                                                                       \
  static Symbol* kSymbol;                                                       \
  static Name* kInstance;                                                       \
  static void Init();                                                           \
                                                                                \
 public:                                                                        \
  static constexpr const auto kSymbolString = (Sym);                            \
  static inline auto Get() -> Name* {                                           \
    ASSERT(kInstance);                                                          \
    return kInstance;                                                           \
  }                                                                             \
  static inline auto GetNativeSymbol() -> Symbol* {                             \
    ASSERT(kSymbol);                                                            \
    return kSymbol;                                                             \
  }

#define DEFINE_NATIVE_PROCEDURE_TYPE(Name) _DEFINE_NATIVE_PROCEDURE_TYPE(Name, #Name)
#define _NATIVE_PROCEDURE_NAMED(Name) class Name : public NativeProcedure

#define _DECLARE_NATIVE_PROCEDURE(Name, Sym)  \
  _NATIVE_PROCEDURE_NAMED(Name) {             \
    _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym); \
  };

#define DECLARE_NATIVE_PROCEDURE(Name)  \
  _NATIVE_PROCEDURE_NAMED(Name) {       \
    DEFINE_NATIVE_PROCEDURE_TYPE(Name); \
  };

#define NATIVE_PROCEDURE_F(Name)                        \
  Name* Name::kInstance = nullptr;                      \
  Symbol* Name::kSymbol = nullptr;                      \
  void Name::Init() {                                   \
    ASSERT(kInstance == nullptr && kSymbol == nullptr); \
    kSymbol = Symbol::New(kSymbolString);               \
    kInstance = new Name();                             \
    ASSERT(kInstance&& kSymbol);                        \
  }                                                     \
  auto Name::ApplyProcedure(const std::vector<Object*>& args) const -> bool

template <class N>
static inline auto IsCallToNative(Symbol* symbol) -> bool {
  ASSERT(symbol);
  return N::GetNativeSymbol()->Equals(symbol);
}

}  // namespace scm

#endif  // SCM_NATIVE_PROCEDURE_H
