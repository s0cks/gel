#ifndef GEL_OBJECT_H
#define GEL_OBJECT_H

#include <exception>
#include <fmt/format.h>
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

#include "gel/allocator.h"
#include "gel/binary_op.h"
#include "gel/common.h"
#include "gel/platform.h"
#include "gel/region.h"
#include "gel/rx.h"
#include "gel/type.h"
#include "gel/type_traits.h"
#include "gel/unary_op.h"

namespace gel {
namespace proc {
class rx_map;
class rx_subscribe;
class rx_buffer;
}  // namespace proc

class Pointer;
class Object;
class PointerVisitor;
class Object : public HeapObject {
  friend class Macro;
  friend class Parser;
  friend class Module;
  friend class Pointer;
  friend class RefBase;
  friend class Procedure;
  friend class Namespace;
  DEFINE_NON_COPYABLE_TYPE(Object)
 protected:
  Object() = default;

  template <typename T>
  static inline void CombineHash(uword& seed, const T& rhs) {
    std::hash<T> hasher;
    seed ^= hasher(rhs) + 0x9e3779b9 + (seed << 6) + (seed >> 2);  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  }

  auto FieldAddrAtOffset(const uword offset) const -> Object** {
    const auto address = ((uword)this) + offset;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    return ((Object**)address);                   // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto FieldAddr(Field* field) const -> Object**;

  virtual void AddChild(Object* rhs) {
    ASSERT(rhs);
    // do nothing
  }

 public:
  ~Object() override = default;
  virtual auto GetType() const -> Class* = 0;
  virtual auto HashCode() const -> uword = 0;
  virtual auto Equals(Object* rhs) const -> bool = 0;

#define DECLARE_BINARY_OP(Name) virtual auto Name(Object* rhs) const -> Object*;
  FOR_EACH_BINARY_OP(DECLARE_BINARY_OP)

  virtual auto Compare(Object* rhs) const -> bool = 0;

  auto GetField(Field* field) const -> Object* {
    ASSERT(field);
    return (*FieldAddr(field));
  }

  void SetField(Field* field, Object* rhs) {
    (*FieldAddr(field)) = rhs;
  }

  virtual auto IsLocal() const -> bool {
    return false;
  }

  virtual auto IsArgument() const -> bool {
    return false;
  }

  virtual auto IsArray() const -> bool {
    return false;
  }

  virtual auto IsAtom() const -> bool {
    return false;
  }

  virtual auto AsExpression() -> expr::Expression* {
    return nullptr;
  }

  virtual auto IsExpression() -> bool {
    return AsExpression() != nullptr;
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
  static auto VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool;

  static inline auto VisitClassPointerPointer(const std::function<bool(Pointer**)>& func) -> bool {
    PointerPointerVisitorWrapper vis = func;
    return VisitClassPointerPointer(&vis);
  }

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};

struct ObjectHasher {
  auto operator()(Object* rhs) const -> size_t {
    ASSERT(rhs);
    return rhs->HashCode();
  }
};

struct ObjectEquals {
  auto operator()(Object* lhs, Object* rhs) const -> bool {
    ASSERT(rhs);
    return lhs->Equals(rhs);
  }
};

struct ObjectComparator {
  auto operator()(Object* lhs, Object* rhs) const -> bool {
    return lhs->Compare(rhs);
  }
};

namespace ir {
class GraphEntryInstr;
}

static inline auto operator<<(std::ostream& stream, Object* rhs) -> std::ostream& {
  return stream << rhs->ToString();
}

#define DECLARE_TYPE(Name)                                                  \
  friend class Class;                                                       \
  friend class Object;                                                      \
  DEFINE_NON_COPYABLE_TYPE(Name)                                            \
 private:                                                                   \
  static Class* kClass;                                                     \
  static void InitClass();                                                  \
  static auto CreateClass() -> Class*;                                      \
                                                                            \
 public:                                                                    \
  static auto New(const ObjectList& args) -> Name*;                         \
  static constexpr const auto kClassId = Class::k##Name##ClassId;           \
  static constexpr const auto kClassName = #Name;                           \
  static auto operator new(const size_t sz)->void*;                         \
  static inline void operator delete(void* ptr) {                           \
    ASSERT(ptr);                                                            \
  }                                                                         \
  static inline auto GetClass() -> Class* {                                 \
    ASSERT(kClass);                                                         \
    return kClass;                                                          \
  }                                                                         \
  static auto VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool; \
                                                                            \
 public:                                                                    \
  auto HashCode() const -> uword override;                                  \
  auto Equals(Object* rhs) const -> bool override;                          \
  auto Compare(Object* rhs) const -> bool override;                         \
  auto GetType() const -> Class* override {                                 \
    return GetClass();                                                      \
  }                                                                         \
  auto ToString() const -> std::string override;                            \
  auto As##Name()->Name* override {                                         \
    return this;                                                            \
  }
}  // namespace gel

#include "gel/class.h"

namespace gel {
class Seq : public Object {
  friend class Object;
  DEFINE_NON_COPYABLE_TYPE(Seq);

 private:
 protected:
  Seq() = default;

 public:
  ~Seq() override = default;
  virtual auto IsEmpty() const -> bool = 0;

  auto HashCode() const -> uword override;
  auto Equals(Object* rhs) const -> bool override;

  auto GetType() const -> Class* override {
    return GetClass();
  }

  auto AsSeq() -> Seq* override {
    return this;
  }

  static auto New(const ObjectList& args) -> Seq*;
  static auto operator new(const size_t sz) -> void*;
  static inline void operator delete(void* ptr) {
    ASSERT(ptr);
  }

 private:
  static Class* kClass;
  static void InitClass();
  static auto CreateClass() -> Class*;

 public:
  static auto VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool;

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};

class Bool : public Object {
 private:
  bool value_;

  explicit Bool(const bool value) :
    Object(),
    value_(value) {}

 public:
  ~Bool() override = default;

  auto Get() const -> bool {
    return value_;
  }

  auto Negate() const -> Bool* {
    return Get() ? False() : True();
  }

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

using RawLong = int64_t;

class Number : public Object {
  friend class Long;
  friend class Double;

 private:
  std::variant<RawLong, double> value_;

 protected:
  explicit Number(const RawLong value) :
    Object(),
    value_(value) {}
  explicit Number(const double value) :
    Object(),
    value_(value) {}

 public:
  ~Number() override = default;

  auto value() const -> const std::variant<RawLong, double>& {
    return value_;
  }

  auto GetLong() const -> RawLong {
    if (std::holds_alternative<double>(value()))
      return static_cast<RawLong>(GetDouble());
    return std::get<RawLong>(value());
  }

  auto GetDouble() const -> double {
    if (std::holds_alternative<RawLong>(value()))
      return static_cast<double>(GetLong());
    return std::get<double>(value());
  }

  auto BitNot() const -> Object*;

  DECLARE_TYPE(Number);

 public:
  static auto New(const RawLong rhs) -> Number*;
  static auto New(const double rhs) -> Number*;
};

class Long : public Number {
 protected:
  explicit Long(const RawLong value) :
    Number(value) {}

 public:
  ~Long() override = default;

  inline auto Get() const -> RawLong {
    return GetLong();
  }

  auto Add(Object* rhs) const -> Object* override;
  auto Subtract(Object* rhs) const -> Object* override;
  auto Multiply(Object* rhs) const -> Object* override;
  auto Divide(Object* rhs) const -> Object* override;
  auto Modulus(Object* rhs) const -> Object* override;
  auto BitAnd(Object* rhs) const -> Object* override;
  auto BitOr(Object* rhs) const -> Object* override;
  auto BitXor(Object* rhs) const -> Object* override;
  auto ShiftLeft(Object* rhs) const -> Object* override;
  auto ShiftRight(Object* rhs) const -> Object* override;

  auto Eq(Object* rhs) const -> Object* override;
  auto GreaterThan(Object* rhs) const -> Object* override;
  auto LessThan(Object* rhs) const -> Object* override;
  DECLARE_TYPE(Long);

 public:
  static inline auto New(const RawLong value) -> Long* {
    return new Long(value);
  }

  static auto Unbox(Object* rhs) -> RawLong;
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

  auto Add(Object* rhs) const -> Object* override;
  auto Subtract(Object* rhs) const -> Object* override;
  auto Multiply(Object* rhs) const -> Object* override;
  auto Divide(Object* rhs) const -> Object* override;
  DECLARE_TYPE(Double);

 public:
  static inline auto New(const double value) -> Double* {
    return new Double(value);
  }
};

class Pair : public Seq {
 public:
  static Field* kFirstField;
  static Field* kSecondField;

 protected:
  explicit Pair(Object* car = nullptr, Object* cdr = nullptr) :
    Seq() {
    if (car)
      SetFirst(car);
    if (cdr)
      SetSecond(cdr);
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;

 public:
  ~Pair() override = default;

  auto GetFirst() const -> Object* {
    return GetField(kFirstField);
  }

  inline auto HasFirst() const -> bool {
    return GetFirst() != nullptr;
  }

  void SetFirst(Object* rhs) {  // TODO: reduce visibility
    ASSERT(rhs);
    SetField(kFirstField, rhs);
  }

  auto GetSecond() const -> Object* {
    return GetField(kSecondField);
  }

  inline auto HasSecond() const -> bool {
    return GetSecond() != nullptr;
  }

  void SetSecond(Object* rhs) {  // TODO: reduce visibility
    ASSERT(rhs);
    SetField(kSecondField, rhs);
  }

  auto IsEmpty() const -> bool override {
    return !HasFirst() && !HasSecond();
  }

  auto IsTuple() const -> bool {
    return HasSecond() && !GetSecond()->IsPair();
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

  static auto VisitEmptyPointerPointer(const std::function<bool(Pointer**)>& vis) -> bool;
  static auto VisitEmptyPointerPointer(PointerPointerVisitor* vis) -> bool;
};

class StringObject : public Object {
  DEFINE_NON_COPYABLE_TYPE(StringObject);

 private:
  std::string value_;

 protected:
  StringObject() = default;
  explicit StringObject(std::string value) :
    Object(),
    value_(std::move(value)) {}

  inline void Set(const std::string& value) {
    value_ = value;
  }

 public:
  ~StringObject() override = default;

  auto Get() const -> const std::string& {
    return value_;
  }

  constexpr auto GetLength() const -> uword {
    return value_.length();
  }

  inline auto IsEmpty() const -> bool {
    return value_.empty();
  }

  auto HashCode() const -> uword override;
  auto Equals(Object* rhs) const -> bool override;
  auto Equals(const std::string& rhs) const -> bool;
};

class String : public StringObject {
 protected:
  String() = default;
  explicit String(const std::string& value) :
    StringObject(value) {}

 public:
  ~String() override = default;
  auto Eq(Object* rhs) const -> Object* override;
  auto Equals(const std::string& rhs) const -> bool;

  auto ToBuffer() const -> Buffer*;
  DECLARE_TYPE(String);

 public:
  inline friend auto operator<<(std::ostream& stream, const String& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

 public:
  static auto New() -> String*;
  static auto New(Symbol* rhs) -> String*;
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

auto PrintValue(std::ostream& stream, Object* value) -> std::ostream&;

#define DEFINE_TYPE_PRED(Name)                     \
  static inline auto Is##Name(Object* rhs)->bool { \
    return rhs && rhs->Is##Name();                 \
  }
FOR_EACH_TYPE(DEFINE_TYPE_PRED)
#undef DEFINE_TYPE_PRED

#define DEFINE_TYPE_CAST(Name)                                 \
  static inline auto To##Name(Object* rhs)->Name* {            \
    return rhs && rhs->Is##Name() ? rhs->As##Name() : nullptr; \
  }
FOR_EACH_TYPE(DEFINE_TYPE_CAST)
#undef DEFINE_TYPE_CAST

static inline auto Null() -> Object* {
  return Pair::Empty();
}

static inline auto IsNull(Object* rhs) -> bool {
  if (!rhs)
    return true;
  return (rhs->IsPair() && rhs->AsPair()->IsEmpty());
}

static inline auto Cons(Object* lhs, Object* rhs) -> Object* {
  ASSERT(lhs);
  ASSERT(rhs);
  return Pair::New(lhs, rhs);
}

static inline auto ToList(const ObjectList& values, const bool reverse = false) -> Object* {
  Object* result = Null();
  if (reverse) {
    for (const auto& next : std::ranges::reverse_view(values)) {
      result = Pair::New(next, result);
    }
  } else {
    for (const auto& next : values) {
      result = Pair::New(next, result);
    }
  }
  ASSERT(result);
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

static inline auto ListFromRange(const int64_t from, const int64_t to) -> gel::Object* {
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
  const auto value = rhs->AsPair()->GetFirst();
  return value ? value : Null();
}

static inline auto Cdr(Object* rhs) -> Object* {
  ASSERT(rhs && rhs->IsPair());
  const auto value = rhs->AsPair()->GetSecond();
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
  (seq->AsPair())->SetFirst(value);
}

static inline void SetCdr(Object* seq, Object* value) {
  ASSERT(seq && seq->IsPair());
  (seq->AsPair())->SetSecond(value);
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
    format_to(ctx.out(), "{}", *(value.GetFirst()));
    auto next = value.GetSecond();
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
      format_to(ctx.out(), "{}", next->AsPair()->GetFirst());
      next = next->AsPair()->GetSecond();
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
