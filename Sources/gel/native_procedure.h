#ifndef GEL_NATIVE_PROCEDURE_H
#define GEL_NATIVE_PROCEDURE_H

#include <type_traits>
#include <variant>

#include "argument.h"
#include "array.h"
#include "boolean.h"
#include "common.h"
#include "error.h"
#include "fmt.h"
#include "native_entry.h"
#include "nil.h"
#include "number.h"
#include "pointer.h"
#include "procedure.h"

namespace gel {
class Runtime;
class NativeFn;
using NativeFnList = std::vector<NativeFn*>;
class NativeFn : public Fn {
  friend class Parser;
  friend class Runtime;
  friend class Interpreter;
  friend class NativeFnEntry;

 private:
  NativeFnEntry* entry_ = nullptr;

  inline void SetEntry(NativeFnEntry* entry) {
    LOG_IF(FATAL, HasEntry()) << "cannot relink " << this << " to: " << (*entry);
    ASSERT(entry);
    entry_ = entry;
  }

  static auto FindOrCreate(Symbol* symbol) -> NativeFn*;

 public:
  static void Link(Symbol* symbol, NativeFnEntry* entry);

 protected:
  explicit NativeFn(Symbol* symbol) :
    Fn(symbol) {}

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~NativeFn() override = default;

  auto IsNative() const -> bool override {
    return true;
  }

  auto GetEntry() const -> NativeFnEntry* {
    return entry_;
  }

  inline auto HasEntry() const -> bool {
    return GetEntry() != nullptr;
  }

  inline auto IsLinked() const -> bool {
    return HasEntry();
  }

  inline auto Apply(const ObjectList args) const -> bool {
    ASSERT(IsLinked());
    return entry_->Apply(std::move(args));
  }

  auto operator()(const ObjectList args) const -> bool {
    return Apply(std::move(args));
  }

  friend auto operator<<(std::ostream& stream, const NativeFn& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

  DECLARE_TYPE(NativeFn);

 private:
  static NativeFnList all_;
  static void InitNatives();

 protected:
  static void Register(NativeFn* native);

 public:
  static void Init();
  static auto Find(const std::string& name) -> NativeFn*;
  static auto Find(Symbol* symbol) -> NativeFn*;

  static inline auto GetAll() -> const NativeFnList& {
    return all_;
  }
};

template <class Native>
static inline auto InitNative() -> Native* {
  Native::Init();
  const auto native = Native::Get();
  ASSERT(native);
  DVLOG(1000) << "initialized " << native;
  return native;
}

#define _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym)             \
  friend class gel::Runtime;                                 \
  friend class NativeFn;                                     \
  DEFINE_NON_COPYABLE_TYPE(Name);                            \
                                                             \
 protected:                                                  \
  auto Apply(const ObjectList& args) const -> bool override; \
                                                             \
 public:                                                     \
  Name() :                                                   \
    NativeFnEntry() {}                                       \
  ~Name() override = default;                                \
                                                             \
 private:                                                    \
  static constexpr const auto kSymbolString = (Sym);         \
  static Symbol* kSymbol;                                    \
  static Name* kInstance;                                    \
                                                             \
 public:                                                     \
  static void Init();                                        \
  static inline auto Get() -> Name* {                        \
    ASSERT(kInstance);                                       \
    return kInstance;                                        \
  }                                                          \
  static inline auto GetNativeSymbol() -> Symbol* {          \
    ASSERT(kSymbol);                                         \
    return kSymbol;                                          \
  }

#define DEFINE_NATIVE_PROCEDURE_TYPE(Name) _DEFINE_NATIVE_PROCEDURE_TYPE(Name, #Name)
#define _NATIVE_PROCEDURE_NAMED(Name)      class Name : public NativeFnEntry

#define _DECLARE_NATIVE_PROCEDURE(Name, Sym)  \
  _NATIVE_PROCEDURE_NAMED(Name) {             \
    _DEFINE_NATIVE_PROCEDURE_TYPE(Name, Sym); \
  };

#define DECLARE_NATIVE_PROCEDURE(Name)  \
  _NATIVE_PROCEDURE_NAMED(Name) {       \
    DEFINE_NATIVE_PROCEDURE_TYPE(Name); \
  };

#define NATIVE_PROCEDURE_F(Name)                        \
  Symbol* Name::kSymbol = nullptr;                      \
  Name* Name::kInstance = nullptr;                      \
  void Name::Init() {                                   \
    ASSERT(kInstance == nullptr && kSymbol == nullptr); \
    DVLOG(100) << "initializing " << #Name << "....";   \
    kInstance = new Name();                             \
    ASSERT(kInstance);                                  \
    kSymbol = Symbol::New(kSymbolString);               \
    ASSERT(kSymbol);                                    \
    NativeFn::Link(kSymbol, kInstance);                 \
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
      return Error::New(fmt::format("Argument #{} is {}", GetIndex(), *Nil::Get()));
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

#define CHECK_NATIVE_ARG(Name) \
  ({                           \
    if (!Name)                 \
      return Throw(Name);      \
  })

#define REQUIRED_NATIVE_ARG(Index, Type, Name) \
  NativeArgument<Index, Type> Name(args);      \
  CHECK_NATIVE_ARG(Name);

}  // namespace gel

namespace fmt {
template <>
struct formatter<gel::NativeFn> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::NativeFn& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.ToString());
  }
};
}  // namespace fmt

#endif  // GEL_NATIVE_PROCEDURE_H
