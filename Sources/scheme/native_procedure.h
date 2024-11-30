#ifndef SCM_NATIVE_PROCEDURE_H
#define SCM_NATIVE_PROCEDURE_H

#include "scheme/common.h"
#include "scheme/error.h"
#include "scheme/instruction.h"
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
  virtual auto Apply(const ObjectList& args) const -> bool = 0;

  template <class T, typename... Args>
  inline auto ReturnNew(Args... args) const -> bool {
    return Return(T::New(args...));
  }

  inline auto Throw(Error* error) const -> bool {
    ASSERT(error);
    LOG(ERROR) << "error: " << error->ToString();
    return Return(error);
  }

  inline auto ThrowError(const std::string& message) const -> bool {
    return Throw(Error::New(message));
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

  auto GetEntry() const -> instr::TargetEntryInstr* {
    return nullptr;  // TODO: remove this stupid function
  }

  DECLARE_TYPE(NativeProcedure);
};

#define _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym)             \
  friend class scm::Runtime;                                 \
  DEFINE_NON_COPYABLE_TYPE(Name);                            \
                                                             \
 protected:                                                  \
  auto Apply(const ObjectList& args) const -> bool override; \
                                                             \
 public:                                                     \
  Name() :                                                   \
    NativeProcedure(kSymbol) {}                              \
  ~Name() override = default;                                \
                                                             \
 private:                                                    \
  static Symbol* kSymbol;                                    \
  static Name* kInstance;                                    \
  static void Init();                                        \
                                                             \
 public:                                                     \
  static constexpr const auto kSymbolString = (Sym);         \
  static inline auto Get() -> Name* {                        \
    ASSERT(kInstance);                                       \
    return kInstance;                                        \
  }                                                          \
  static inline auto GetNativeSymbol() -> Symbol* {          \
    ASSERT(kSymbol);                                         \
    return kSymbol;                                          \
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
  auto Name::Apply(const ObjectList& args) const -> bool

template <class N>
static inline auto IsCallToNative(Symbol* symbol) -> bool {
  ASSERT(symbol);
  return N::GetNativeSymbol()->Equals(symbol);
}

template <const uword Index, class T = Object, const bool Required = true>
class NativeArgument {
  DEFINE_NON_COPYABLE_TYPE(NativeArgument);

 private:
  scm::Object* value_ = nullptr;

  void SetValue(T* rhs) {
    ASSERT(rhs);
    value_ = rhs;
  }

 public:
  explicit NativeArgument(const ObjectList& args) {
    if (Index < 0 || Index > args.size()) {
      if (Required) {
        value_ = Error::New("Hello World");
        return;
      }
      value_ = Null();
      return;
    }
    const auto value = args[Index];
    if (!value && Required) {
      value_ = Error::New(fmt::format("arg #{} to not be '()", Index));
      return;
    }

    if (!value->GetType()->IsInstanceOf(T::GetClass())) {
      value_ = Error::New(
          fmt::format("arg #{} `{}` is expected to be an instance of: `{}`", Index, (*value), T::GetClass()->GetName()->Get()));
      return;
    }
    SetValue((T*)value);
  }
  ~NativeArgument() = default;

  inline auto HasValue() const -> bool {
    return value_ != nullptr;
  }

  auto HasError() const -> bool {
    return (Required && !HasValue()) || (HasValue() && value_->IsError());
  }

  auto GetValue() const -> T* {
    return (T*)value_;
  }

  auto GetIndex() const -> uword {
    return Index;
  }

  auto IsRequired() const -> bool {
    return Required;
  }

  auto IsOptional() const -> bool {
    return !Required;
  }

  auto GetType() const -> Class* {
    return T::GetClass();
  }

  auto GetError() const -> Error* {
    return value_ && value_->IsError() ? value_->AsError() : Error::New("value is null");
  }

  operator bool() const {
    return !HasError();
  }

  operator T*() const {
    return GetValue();
  }

  auto operator->() -> T* {
    return GetValue();
  }
};

template <const uword Index, class T>
using OptionalNativeArgument = NativeArgument<Index, T, false>;

template <const uword Index, class T>
using RequiredNativeArgument = NativeArgument<Index, T>;

}  // namespace scm

#endif  // SCM_NATIVE_PROCEDURE_H
