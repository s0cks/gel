#ifndef GEL_OBJECT_H
#define GEL_OBJECT_H

#include <fmt/format.h>

#include <exception>
#include <functional>
#include <numeric>
#include <ostream>
#include <ranges>
#include <rpp/observables/dynamic_observable.hpp>
#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/observers/observer.hpp>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "gel/common.h"
#include "gel/pointer.h"
#include "gel/rx.h"
#include "gel/type.h"
#include "gel/type_traits.h"
#include "gmock/gmock.h"

namespace gel {
namespace proc {
class rx_map;
class rx_subscribe;
class rx_buffer;
}  // namespace proc

class Pointer;
class Datum;
class PointerVisitor;
class Object {
  DEFINE_NON_COPYABLE_TYPE(Object)
 protected:
  Object() = default;

  virtual auto VisitPointers(PointerVisitor* vis) -> bool {
    ASSERT(vis);
    // do nothing
    return true;
  }

  virtual auto VisitPointers(PointerPointerVisitor* vis) -> bool {
    ASSERT(vis);
    // do nothing
    return true;
  }

 public:
  virtual ~Object() = default;
  virtual auto GetType() const -> Class* = 0;
  virtual auto Equals(Object* rhs) const -> bool = 0;
  virtual auto ToString() const -> std::string = 0;

  auto AsObject() const -> Object* {
    return ((Object*)this);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto raw_ptr() const -> Pointer*;

  inline auto GetStartingAddress() const -> uword {
    return (uword)this;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto GetStartingAddressPointer() const -> void* {
    return ((void*)GetStartingAddress());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  virtual auto AsDatum() -> Datum* {
    return nullptr;
  }

  auto IsDatum() -> bool {
    return AsDatum() != nullptr;
  }

  virtual auto IsAtom() const -> bool {
    return false;
  }

  virtual auto IsArray() const -> bool {
    return false;
  }

#define DEFINE_TYPE_CHECK(Name)    \
  virtual auto As##Name()->Name* { \
    return nullptr;                \
  }                                \
  auto Is##Name()->bool {          \
    return As##Name() != nullptr;  \
  }
  FOR_EACH_TYPE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
  static constexpr const auto kClassName = "Object";

 private:
  static Class* kClass;
  static auto CreateClass() -> Class*;
  static void InitClass();

 public:
  static void Init();

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};

namespace ir {
class GraphEntryInstr;
}

class Executable {
  DEFINE_NON_COPYABLE_TYPE(Executable);

 private:
  ir::GraphEntryInstr* entry_ = nullptr;

 protected:
  Executable() = default;

  void SetEntry(ir::GraphEntryInstr* entry) {
    ASSERT(entry);
    entry_ = entry;
  }

 public:
  virtual ~Executable() = default;

  auto GetEntry() const -> ir::GraphEntryInstr* {
    return entry_;
  }

  inline auto HasEntry() const -> bool {
    return GetEntry() != nullptr;
  }

  inline auto IsCompiled() const -> bool {
    return HasEntry();
  }
};

static inline auto operator<<(std::ostream& stream, Object* rhs) -> std::ostream& {
  return stream << rhs->ToString();
}

#define DECLARE_TYPE(Name)                          \
  friend class Object;                              \
  DEFINE_NON_COPYABLE_TYPE(Name)                    \
 private:                                           \
  static Class* kClass;                             \
  static void InitClass();                          \
  static auto CreateClass() -> Class*;              \
                                                    \
 public:                                            \
  static auto New(const ObjectList& args) -> Name*; \
  static constexpr const auto kClassName = #Name;   \
  static auto operator new(const size_t sz)->void*; \
  static inline void operator delete(void* ptr) {   \
    ASSERT(ptr);                                    \
  }                                                 \
  static inline auto GetClass() -> Class* {         \
    ASSERT(kClass);                                 \
    return kClass;                                  \
  }                                                 \
                                                    \
 public:                                            \
  auto Equals(Object* rhs) const -> bool override;  \
  auto GetType() const -> Class* override {         \
    return GetClass();                              \
  }                                                 \
  auto ToString() const -> std::string override;    \
  auto As##Name()->Name* override {                 \
    return this;                                    \
  }

class Datum : public Object {
  DEFINE_NON_COPYABLE_TYPE(Datum);

 protected:
  Datum() = default;

 public:
  ~Datum() override = default;

  auto IsAtom() const -> bool override {
    return true;
  }

  auto AsDatum() -> Datum* override {
    return this;
  }

  virtual auto Add(Datum* rhs) const -> Datum*;
  virtual auto Sub(Datum* rhs) const -> Datum*;
  virtual auto Mul(Datum* rhs) const -> Datum*;
  virtual auto Div(Datum* rhs) const -> Datum*;
  virtual auto Mod(Datum* rhs) const -> Datum*;
  virtual auto And(Datum* rhs) const -> Datum*;
  virtual auto Or(Datum* rhs) const -> Datum*;
  virtual auto Compare(Datum* rhs) const -> int;
};
}  // namespace gel

#include "gel/class.h"

namespace gel {
class Bool : public Datum {
 private:
  bool value_;

  explicit Bool(const bool value) :
    Datum(),
    value_(value) {}

 public:
  ~Bool() override = default;

  auto Get() const -> bool {
    return value_;
  }

  auto And(Datum* rhs) const -> Datum* override;
  auto Or(Datum* rhs) const -> Datum* override;

  DECLARE_TYPE(Bool);

 private:
  static void Init();

 public:
  static auto New(const bool value) -> Bool*;

  static inline auto NewTrue() -> Bool* {
    return New(true);
  }

  static inline auto NewFalse() -> Bool* {
    return New(false);
  }

  static auto True() -> Bool*;
  static auto False() -> Bool*;

  static inline auto Box(const bool rhs) -> Bool* {
    return rhs ? True() : False();
  }

  static inline auto Unbox(Bool* rhs) -> bool {
    ASSERT(rhs);
    return rhs->Get();
  }
};

class Number : public Datum {
  friend class Long;
  friend class Double;

 private:
  std::variant<uint64_t, double> value_;

 protected:
  explicit Number(const uint64_t value) :
    Datum(),
    value_(value) {}
  explicit Number(const double value) :
    Datum(),
    value_(value) {}

 public:
  ~Number() override = default;

  auto value() const -> const std::variant<uint64_t, double>& {
    return value_;
  }

  auto GetLong() const -> uint64_t {
    return std::get<uint64_t>(value());
  }

  auto GetDouble() const -> double {
    return std::get<double>(value());
  }

  DECLARE_TYPE(Number);

 public:
  static auto New(const uint64_t rhs) -> Number*;
  static auto New(const double rhs) -> Number*;
};

class Long : public Number {
 protected:
  explicit Long(const uint64_t value) :
    Number(value) {}

 public:
  ~Long() override = default;

  inline auto Get() const -> uint64_t {
    return GetLong();
  }

  auto Add(Datum* rhs) const -> Datum* override;
  auto Sub(Datum* rhs) const -> Datum* override;
  auto Mul(Datum* rhs) const -> Datum* override;
  auto Div(Datum* rhs) const -> Datum* override;
  auto Mod(Datum* rhs) const -> Datum* override;
  auto Compare(Datum* rhs) const -> int override;
  DECLARE_TYPE(Long);

 public:
  static inline auto New(const uintptr_t value) -> Long* {
    return new Long(value);
  }

  static auto Unbox(Object* rhs) -> uint64_t;
};

class Double : public Number {
 protected:
  Double(const double value) :
    Number(value) {}

 public:
  ~Double() override = default;

  inline auto Get() const -> double {
    return GetDouble();
  }

  auto Add(Datum* rhs) const -> Datum* override;
  auto Sub(Datum* rhs) const -> Datum* override;
  auto Mul(Datum* rhs) const -> Datum* override;
  auto Div(Datum* rhs) const -> Datum* override;
  DECLARE_TYPE(Double);

 public:
  static inline auto New(const double value) -> Double* {
    return new Double(value);
  }
};

class Pair : public Datum {
 private:
  Object* car_;
  Object* cdr_;

 protected:
  explicit Pair(Object* car = nullptr, Object* cdr = nullptr) :
    car_(car),
    cdr_(cdr) {}

  auto VisitPointers(PointerVisitor* vis) -> bool override;

 public:
  ~Pair() override = default;

  auto GetCar() const -> Object* {
    return car_;
  }

  inline auto HasCar() const -> bool {
    return GetCar() != nullptr;
  }

  void SetCar(Object* rhs) {
    ASSERT(rhs);
    car_ = rhs;
  }

  auto GetCdr() const -> Object* {
    return cdr_;
  }

  inline auto HasCdr() const -> bool {
    return GetCdr() != nullptr;
  }

  void SetCdr(Object* rhs) {
    ASSERT(rhs);
    cdr_ = rhs;
  }

  inline auto IsEmpty() const -> bool {
    return !HasCar() && !HasCdr();
  }

  DECLARE_TYPE(Pair);

 public:
  static auto Empty() -> Pair*;
  static inline auto NewEmpty() -> Pair* {
    return new Pair();
  }
  static inline auto New(Object* car, Object* cdr) -> Pair* {
    return new Pair(car, cdr);
  }
};

class StringObject : public Datum {
  DEFINE_NON_COPYABLE_TYPE(StringObject);

 private:
  std::string value_;

 protected:
  StringObject() = default;
  explicit StringObject(std::string value) :
    Datum(),
    value_(std::move(value)) {}

  inline void Set(const std::string& value) {
    value_ = value;
  }

 public:
  ~StringObject() override = default;

  auto Get() const -> const std::string& {
    return value_;
  }

  inline auto IsEmpty() const -> bool {
    return value_.empty();
  }

  auto Equals(const std::string& rhs) const -> bool;
};

class String : public StringObject {
 protected:
  String() = default;
  explicit String(const std::string& value) :
    StringObject(value) {}

 public:
  ~String() override = default;

  DECLARE_TYPE(String);

 public:
  static auto New() -> String*;
  static inline auto New(const std::string& value) -> String* {
    return new String(value);
  }
  static inline auto Unbox(Object* rhs) -> const std::string& {
    ASSERT(rhs && rhs->IsString());
    return rhs->AsString()->Get();
  }

  static auto Empty() -> String*;
  static auto ValueOf(Object* rhs) -> String*;
};

class Symbol : public StringObject {
 public:
  struct Comparator {
    auto operator()(Symbol* lhs, Symbol* rhs) const -> bool {
      ASSERT(lhs && rhs);
      return lhs->Get() < rhs->Get();
    }
  };

 private:
  explicit Symbol(const std::string& value) :
    StringObject(value) {}

 public:
  ~Symbol() override = default;

  DECLARE_TYPE(Symbol);

 public:
  static auto New(const std::string& rhs) -> Symbol*;
};

using SymbolList = std::vector<Symbol*>;
using SymbolSet = std::unordered_set<Symbol*, Symbol::Comparator>;

auto PrintValue(std::ostream& stream, Object* value) -> std::ostream&;

#define DEFINE_TYPE_PRED(Name)                     \
  static inline auto Is##Name(Object* rhs)->bool { \
    return rhs && rhs->Is##Name();                 \
  }
DEFINE_TYPE_PRED(Array);
FOR_EACH_TYPE(DEFINE_TYPE_PRED)
#undef DEFINE_TYPE_PRED

#define DEFINE_TYPE_CAST(Name)                                 \
  static inline auto To##Name(Object* rhs)->Name* {            \
    return rhs && rhs->Is##Name() ? rhs->As##Name() : nullptr; \
  }
FOR_EACH_TYPE(DEFINE_TYPE_CAST)
#undef DEFINE_TYPE_CAST

static inline auto ToString(Symbol* rhs) -> String* {
  return String::New(rhs->Get());
}

static inline auto Null() -> Object* {
  return Pair::Empty();
}

static inline auto IsNull(Object* rhs) -> bool {
  return !rhs || (rhs->IsPair() && rhs->AsPair()->IsEmpty());
}

static inline auto BinaryAnd(Object* lhs, Object* rhs) -> Datum* {
  ASSERT(lhs && lhs->IsDatum());
  ASSERT(rhs && rhs->IsDatum());
  return lhs->AsDatum()->And(rhs->AsDatum());
}

static inline auto Cons(Object* lhs, Object* rhs) -> Datum* {
  ASSERT(lhs);
  ASSERT(rhs);
  return Pair::New(lhs, rhs);
}

static inline auto ToList(const ObjectList& values) -> Object* {
  Object* result = Null();
  for (const auto& next : values) {
    result = Pair::New(next, result);
  }
  return result;
}

template <class Iter>
static inline auto ToList(Iter& iter) -> Object* {
  Object* result = Null();
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    result = Pair::New(next, result);
  }
  return result;
}

static inline auto ListFromRange(const uint64_t from, const uint64_t to) -> gel::Object* {
  auto first = std::min(from, to);
  auto last = std::max(from, to);
  Object* result = Null();
  for (auto idx = last; idx >= first; idx--) {
    result = Pair::New(Long::New(idx), result);
    if (idx == 0)
      break;
  }
  return result;
}

template <class Iter, typename T>
static inline auto ToList(Iter& iter, const std::function<Object*(T)>& map) -> Object* {
  Object* result = Null();
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    result = Pair::New(map(next), result);
  }
  return result;
}

static inline auto Car(Object* rhs) -> Object* {
  ASSERT(rhs && rhs->IsPair());
  const auto value = rhs->AsPair()->GetCar();
  return value ? value : Null();
}

static inline auto Cdr(Object* rhs) -> Object* {
  ASSERT(rhs && rhs->IsPair());
  const auto value = rhs->AsPair()->GetCdr();
  return value ? value : Null();
}

static inline auto Truth(gel::Object* rhs) -> bool {
  ASSERT(rhs);
  if (rhs->IsBool())
    return rhs->AsBool()->Get();
  return !IsNull(rhs);
}

static inline auto Not(Object* rhs) -> Object* {
  ASSERT(rhs);
  return Truth(rhs) ? Bool::False() : Bool::True();
}

static inline void SetCar(Object* seq, Object* value) {
  ASSERT(seq && seq->IsPair());
  (seq->AsPair())->SetCar(value);
}

static inline void SetCdr(Object* seq, Object* value) {
  ASSERT(seq && seq->IsPair());
  (seq->AsPair())->SetCdr(value);
}

template <typename T>
static inline auto Stringify(std::ostream& stream, const std::vector<T*>& values,
                             std::enable_if_t<gel::has_to_string<T>::value>* = nullptr) -> std::ostream& {
  auto remaining = values.size();
  for (const auto& value : values) {
    stream << (value)->ToString();
    if (--remaining >= 1)
      stream << ", ";
  }
  stream << "]";
  return stream;
}

static inline auto operator<<(std::ostream& stream, const ObjectList& values) -> std::ostream& {
  return Stringify(stream, values);
}
}  // namespace gel

#ifdef GEL_ENABLE_RX
#include "gel/rx_object.h"
#endif  // GEL_ENABLE_RX

namespace fmt {
template <>
struct formatter<gel::Symbol> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Symbol& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.Get());
  }
};

template <>
struct formatter<gel::String> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::String& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "\"{}\"", value.Get());
  }
};

template <>
struct formatter<gel::Long> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Long& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.Get());
  }
};

template <>
struct formatter<gel::Double> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Double& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.Get());
  }
};

template <>
struct formatter<gel::Pair> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Pair& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    ctx.out() << "(";
    if (value.IsEmpty()) {
      ctx.out() << ")";
      return ctx.out();
    }
    format_to(ctx.out(), "{}", *(value.GetCar()));
    auto next = value.GetCdr();
    do {
      if (gel::IsNull(next)) {
        ctx.out() << ")";
        return ctx.out();
      }
      if (!next->IsPair()) {
        ctx.out() << " ";
        format_to(ctx.out(), "{}", (*next));
        ctx.out() << ")";
        return ctx.out();
      }
      ctx.out() << " ";
      format_to(ctx.out(), "{}", next->AsPair()->GetCar());
      next = next->AsPair()->GetCdr();
    } while (true);
  }
};

template <>
struct formatter<gel::Object> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Object& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.ToString());
  }
};
}  // namespace fmt

#endif  // GEL_OBJECT_H
