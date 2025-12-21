#ifndef GEL_NATIVE_ENTRY_H
#define GEL_NATIVE_ENTRY_H

#include "boolean.h"
#include "common.h"
#include "error.h"
#include "nil.h"
#include "number.h"
#include "object.h"
#include "str.h"

namespace gel {
template <const uword Index, class T = Object, const bool Required = true>
class NativeArgument {
  DEFINE_NON_COPYABLE_TYPE(NativeArgument);

 private:
  gel::Object* value_ = nullptr;

 protected:
  void SetValue(T* rhs) {
    ASSERT(rhs);
    value_ = rhs;
  }

  inline void SetError(Error* error) {
    ASSERT(error);
    value_ = error;
  }

  inline void SetError(const std::string& message) {
    ASSERT(!message.empty());
    return SetError(Error::New(message));
  }

 public:
  explicit NativeArgument(const ObjectList& args) {
    ASSERT(Index >= 0 && (Index <= args.size() || !Required));
    const auto value = args[Index];
    if (!value) {
      if (Required)
        SetError(fmt::format("arg #{} to not be '()", Index));
      return;
    }
    ASSERT(value);
    if (!value->GetType()->IsInstanceOf(T::GetClass())) {
      SetError(fmt::format("arg #{} `{}` is expected to be an instance of: `{}`", Index, (*value),
                           T::GetClass()->GetName()->Get()));
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

  auto GetValueOr(T* rhs) const -> T* {
    return HasValue() ? GetValue() : rhs;
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

  operator T&() const {
    ASSERT(HasValue());
    return *GetValue();
  }
};

template <const uword Index, class T = Object>
using OptionalNativeArgument = NativeArgument<Index, T, false>;

template <const uword Index, class T>
using RequiredNativeArgument = NativeArgument<Index, T>;

class NativeFn;
class NativeFnEntry {
  friend class Runtime;
  friend class EffectVisitor;
  friend class NativeFn;
  DEFINE_NON_COPYABLE_TYPE(NativeFnEntry);

 private:
  NativeFn* native_ = nullptr;

  void SetNative(NativeFn* native) {
    ASSERT(native);
    native_ = native;
  }

 protected:
  NativeFnEntry() = default;
  virtual auto Apply(const ObjectList& args) const -> bool = 0;

  auto Return(Object* rhs = Nil::Get()) const -> bool;
  inline auto ReturnNull() const -> bool {
    return Return(Nil::Get());
  }

  template <class T, typename... Args>
  inline auto ReturnNew(Args... args) const -> bool {
    return Return(T::New(args...));
  }

  inline auto ReturnString(const std::string& rhs) const -> bool {
    return ReturnNew<String>(rhs);
  }

  inline auto ReturnString(const std::stringstream& rhs) const -> bool {
    return ReturnString(rhs.str());
  }

  inline auto ReturnBool(const bool rhs) const -> bool {
    return Return(Bool::Box(rhs));
  }

  inline auto ReturnTrue() const -> bool {
    return Return(Bool::True());
  }

  inline auto ReturnFalse() const -> bool {
    return Return(Bool::False());
  }

  inline auto ReturnLong(const RawLong rhs = 0) const -> bool {
    return ReturnNew<Long>(rhs);
  }

  inline auto ReturnLong(const uword rhs) const -> bool {
    return ReturnLong(static_cast<RawLong>(rhs));
  }

  inline auto ReturnDouble(const double rhs) const -> bool {
    return ReturnNew<Double>(rhs);
  }

  inline auto Throw(Error* error) const -> bool {
    ASSERT(error);
    return Return(error);
  }

  inline auto ThrowError(const std::string& message) const -> bool {
    return Throw(Error::New(message));
  }

  inline auto ThrowError(const std::stringstream& ss) const -> bool {
    return ThrowError(ss.str());
  }

  auto ThrowNotImplementedError() const -> bool;

  inline auto DoNothing() const -> bool {
    return true;
  }

  template <const uword Index, class T, const bool Required = true>
  inline auto Throw(const NativeArgument<Index, T, Required>& arg) const -> bool {
    ASSERT(!arg);
    return Throw(arg.GetError());
  }

 public:
  virtual ~NativeFnEntry() = default;

  auto GetNative() const -> NativeFn* {
    return native_;
  }

  inline auto HasNative() const -> bool {
    return GetNative() != nullptr;
  }

  inline auto IsBound() const -> bool {
    return HasNative();
  }

  friend auto operator<<(std::ostream& stream, const NativeFnEntry& rhs) -> std::ostream& {
    NOT_IMPLEMENTED(ERROR);
    return stream << "NativeFnEntry()";
  }
};
}  // namespace gel

#endif  // GEL_NATIVE_ENTRY_H
