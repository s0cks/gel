#ifndef GEL_NATIVE_PROCEDURE_H
#define GEL_NATIVE_PROCEDURE_H

#include <__type_traits/remove_pointer.h>

#include <type_traits>
#include <variant>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/error.h"
#include "gel/instruction.h"
#include "gel/object.h"
#include "gel/procedure.h"

namespace gel {
class Runtime;
class NativeProcedure;
using NativeProcedureList = std::vector<NativeProcedure*>;
class NativeProcedure : public Procedure {
  friend class Parser;
  friend class Runtime;
  friend class Interpreter;

 private:
  ArgumentSet args_{};
  String* docs_ = nullptr;

  template <class Native>
  static inline void InitNative() {
    Native::Init();
    const auto native = Native::Get();
    ASSERT(native);
    DVLOG(1000) << "initialized " << native;
  }

 protected:
  explicit NativeProcedure(Symbol* symbol) :
    Procedure(symbol) {}

  auto Return(Object* rhs = Null()) const -> bool;
  virtual auto Apply(const ObjectList& args) const -> bool = 0;

  inline auto ReturnNull() const -> bool {
    return Return(Null());
  }

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

  void SetArgs(const ArgumentSet& args) {
    args_ = args;
  }

  void SetDocs(String* rhs) {
    ASSERT(rhs);
    docs_ = rhs;
  }

 public:
  ~NativeProcedure() override = default;

  auto IsNative() const -> bool override {
    return true;
  }

  auto GetEntry() const -> ir::TargetEntryInstr* {
    return nullptr;  // TODO: remove this stupid function
  }

  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetDocs() const -> String* {
    return docs_;
  }

  inline auto HasDocs() const -> bool {
    return GetDocs() != nullptr;
  }

  DECLARE_TYPE(NativeProcedure);

 private:
  static NativeProcedureList all_;

 protected:
  static void Register(NativeProcedure* native);

 public:
  static void Init();
  static auto Find(const std::string& name) -> NativeProcedure*;
  static auto Find(Symbol* symbol) -> NativeProcedure*;

  static inline auto GetAll() -> const NativeProcedureList& {
    return all_;
  }
};

#define _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym)             \
  friend class gel::Runtime;                                 \
  friend class NativeProcedure;                              \
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
#define _NATIVE_PROCEDURE_NAMED(Name)      class Name : public NativeProcedure

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
    NativeProcedure::Register(kInstance);               \
    ASSERT(kInstance&& kSymbol);                        \
  }                                                     \
  auto Name::Apply(const ObjectList& args) const -> bool

template <class N>
static inline auto IsCallToNative(Symbol* symbol) -> bool {
  ASSERT(symbol);
  return N::GetNativeSymbol()->Equals(symbol);
}

class NativeArgumentBase {
  DEFINE_NON_COPYABLE_TYPE(NativeArgumentBase);

 protected:
  NativeArgumentBase() = default;

 public:
  virtual ~NativeArgumentBase() = default;
  virtual auto GetIndex() const -> uword = 0;
  virtual auto GetType() const -> Class* = 0;
  virtual auto HasValue() const -> bool = 0;
  virtual auto IsRequired() const -> bool = 0;
  virtual auto HasError() const -> bool = 0;
  virtual auto GetError() const -> Error* = 0;

  inline auto IsOptional() const -> bool {
    return !IsRequired();
  }
};

template <const uword Index, const bool Required, class... Types>
class VariantNativeArgument : public NativeArgumentBase {
  DEFINE_NON_COPYABLE_TYPE(VariantNativeArgument);

 private:
  std::optional<gel::Object*> value_{};
  ClassList types_{};

 public:
  VariantNativeArgument(const ObjectList& args) :
    NativeArgumentBase() {
    (..., types_.push_back(Types::GetClass()));
    if (Index < 0 || Index >= args.size()) {
      if (Required) {
        value_ = {Error::New("Hello World")};
        return;
      }
      return;
    }
    const auto value = args[Index];
    if (!value) {
      if (Required) {
        value_ = {Error::New(fmt::format("arg #{} to not be '()", Index))};
      }
      return;
    }
    ASSERT(value);
    const auto actual_type = std::find_if(std::begin(types_), std::end(types_), [value](Class* cls) {
      ASSERT(cls);
      return value->GetType()->IsInstanceOf(cls);
    });
    if (actual_type == std::end(types_) || (*actual_type) == nullptr) {
      value_ = Error::New(fmt::format("arg #{} `{}` is expected to be an instance of", Index, (*value)));
      return;
    }
    value_ = {value};
  }
  ~VariantNativeArgument() override = default;

  auto GetType() const -> Class* override {
    if (!value_)
      return nullptr;
    return (*value_)->GetType();
  }

  auto GetTypes() const -> const ClassList& {
    return types_;
  }

  auto HasValue() const -> bool override {
    return (bool)value_;
  }

  auto HasError() const -> bool override {
    return IsVariant<Error>();
  }

  auto GetIndex() const -> uword override {
    return Index;
  }

  auto IsRequired() const -> bool override {
    return Required;
  }

  template <class T>
  auto IsVariant() const -> bool {
    DLOG(INFO) << "value: " << (*value_)->GetClass();
    return HasValue() && (*value_)->GetClass()->Equals(T::GetClass());
  }

  template <class T>
  auto Get() const -> T* {
    ASSERT(IsVariant<T>());
    return (T*)(*value_);
  }

  auto GetError() const -> Error* override {
    if (!value_)
      return Error::New(fmt::format("Argument #{} is {}", GetIndex(), *Null()));
    ASSERT(HasError());
    return (*value_)->AsError();
  }

  operator bool() const {
    return !HasError();
  }
};

template <const uword Index, class... Types>
using RequiredVariantNativeArgument = VariantNativeArgument<Index, true, Types...>;

template <const uword Index, class... Types>
using OptionalVariadicNativeArgument = VariantNativeArgument<Index, false, Types...>;

template <const uword Index, class T = Object, const bool Required = true>
class NativeArgument {
  DEFINE_NON_COPYABLE_TYPE(NativeArgument);

 private:
  gel::Object* value_ = nullptr;

  void SetValue(T* rhs) {
    ASSERT(rhs);
    value_ = rhs;
  }

 public:
  explicit NativeArgument(const ObjectList& args) {
    if (Index < 0 || Index >= args.size()) {
      if (Required) {
        value_ = Error::New("Hello World");
        return;
      }
      return;
    }
    const auto value = args[Index];
    if (!value) {
      if (Required) {
        value_ = Error::New(fmt::format("arg #{} to not be '()", Index));
      }
      return;
    }

    if (!value->GetType()->IsInstanceOf(T::GetClass())) {
      value_ = Error::New(
          fmt::format("arg #{} `{}` is expected to be an instance of: `{}`", Index, (*value), T::GetClass()->GetName()->Get()));
      return;
    }
    SetValue((T*)value);
  }
  virtual ~NativeArgument() = default;

  inline auto HasValue() const -> bool {
    return value_ != nullptr;
  }

  auto HasError() const -> bool {
    return (Required && !HasValue()) || (HasValue() && value_->IsError());
  }

  virtual auto GetValue() const -> T* {
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

}  // namespace gel

#endif  // GEL_NATIVE_PROCEDURE_H
