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
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/rx.h"
#include "gel/section.h"
#include "gel/type.h"
#include "gel/type_traits.h"

namespace gel {
namespace proc {
class rx_map;
class rx_subscribe;
class rx_buffer;
}  // namespace proc

class Pointer;
class Object;
class PointerVisitor;
class Object {
  friend class Pointer;
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

 public:
  virtual ~Object() = default;
  virtual auto GetType() const -> Class* = 0;
  virtual auto HashCode() const -> uword = 0;
  virtual auto Equals(Object* rhs) const -> bool = 0;
  virtual auto ToString() const -> std::string = 0;
  virtual auto Add(Object* rhs) const -> Object*;
  virtual auto Sub(Object* rhs) const -> Object*;
  virtual auto Mul(Object* rhs) const -> Object*;
  virtual auto Div(Object* rhs) const -> Object*;
  virtual auto Mod(Object* rhs) const -> Object*;
  virtual auto And(Object* rhs) const -> Object*;
  virtual auto Or(Object* rhs) const -> Object*;
  virtual auto Compare(Object* rhs) const -> int;

  auto GetField(Field* field) const -> Object* {
    ASSERT(field);
    return (*FieldAddr(field));
  }

  void SetField(Field* field, Object* rhs) {
    (*FieldAddr(field)) = rhs;
  }

  auto raw_ptr() const -> Pointer*;

  inline auto GetStartingAddress() const -> uword {
    return (uword)this;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto GetStartingAddressPointer() const -> void* {
    return ((void*)GetStartingAddress());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  virtual auto IsAtom() const -> bool {
    return false;
  }

  virtual auto IsArray() const -> bool {
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

struct ObjectComparator {
  auto operator()(Object* lhs, Object* rhs) const -> bool {
    return lhs->Equals(rhs);
  }
};

namespace ir {
class GraphEntryInstr;
}

class Executable {
  friend class FlowGraphCompiler;
  DEFINE_NON_COPYABLE_TYPE(Executable);

 private:
  Region code_{};
#ifdef GEL_DEBUG
  uword compile_time_ns_ = 0;

  void SetCompileTime(const uword ns) {
    compile_time_ns_ = ns;
  }
#endif  // GEL_DEBUG

 protected:
  Executable() = default;

  void SetCodeRegion(const Region& rhs) {
    code_ = rhs;
  }

 public:
  virtual ~Executable() = default;

  auto GetCode() const -> const Region& {
    return code_;
  }

  inline auto IsCompiled() const -> bool {
    return GetCode().IsAllocated();
  }

#ifdef GEL_DEBUG
  auto GetCompileTime() const -> uword {
    return compile_time_ns_;
  }
#endif  // GEL_DEBUG
};

static inline auto operator<<(std::ostream& stream, Object* rhs) -> std::ostream& {
  return stream << rhs->ToString();
}

#define DECLARE_TYPE(Name)                                        \
  friend class Class;                                             \
  friend class Object;                                            \
  DEFINE_NON_COPYABLE_TYPE(Name)                                  \
 private:                                                         \
  static Class* kClass;                                           \
  static void InitClass();                                        \
  static auto CreateClass() -> Class*;                            \
                                                                  \
 public:                                                          \
  static auto New(const ObjectList& args) -> Name*;               \
  static constexpr const auto kClassId = Class::k##Name##ClassId; \
  static constexpr const auto kClassName = #Name;                 \
  static auto operator new(const size_t sz)->void*;               \
  static inline void operator delete(void* ptr) {                 \
    ASSERT(ptr);                                                  \
  }                                                               \
  static inline auto GetClass() -> Class* {                       \
    ASSERT(kClass);                                               \
    return kClass;                                                \
  }                                                               \
                                                                  \
 public:                                                          \
  auto HashCode() const -> uword override;                        \
  auto Equals(Object* rhs) const -> bool override;                \
  auto GetType() const -> Class* override {                       \
    return GetClass();                                            \
  }                                                               \
  auto ToString() const -> std::string override;                  \
  auto As##Name()->Name* override {                               \
    return this;                                                  \
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

  auto And(Object* rhs) const -> Object* override;
  auto Or(Object* rhs) const -> Object* override;

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

class Number : public Object {
  friend class Long;
  friend class Double;

 private:
  std::variant<uint64_t, double> value_;

 protected:
  explicit Number(const uint64_t value) :
    Object(),
    value_(value) {}
  explicit Number(const double value) :
    Object(),
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

  auto Add(Object* rhs) const -> Object* override;
  auto Sub(Object* rhs) const -> Object* override;
  auto Mul(Object* rhs) const -> Object* override;
  auto Div(Object* rhs) const -> Object* override;
  auto Mod(Object* rhs) const -> Object* override;
  auto Compare(Object* rhs) const -> int override;
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

  auto Add(Object* rhs) const -> Object* override;
  auto Sub(Object* rhs) const -> Object* override;
  auto Mul(Object* rhs) const -> Object* override;
  auto Div(Object* rhs) const -> Object* override;
  DECLARE_TYPE(Double);

 public:
  static inline auto New(const double value) -> Double* {
    return new Double(value);
  }
};

class Pair : public Seq {
 private:
  Object* car_;
  Object* cdr_;

 protected:
  explicit Pair(Object* car = nullptr, Object* cdr = nullptr) :
    Seq(),
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

  auto IsEmpty() const -> bool override {
    return !HasCar() && !HasCdr();
  }

  auto IsTuple() const -> bool {
    return HasCdr() && !GetCdr()->IsPair();
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

  auto Equals(const std::string& rhs) const -> bool;
  DECLARE_TYPE(String);

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

class Set : public Object {
 public:
  using StorageType = std::unordered_set<Object*, ObjectHasher, ObjectComparator>;

 private:
  StorageType data_;

 protected:
  Set(const StorageType& data) :
    Object(),
    data_(data) {}

  inline auto Find(Object* rhs) const -> StorageType::const_iterator {
    return data().find(rhs);
  }

 public:
  ~Set() override = default;

  auto data() const -> const StorageType& {
    return data_;
  }

  auto GetSize() const -> uword {
    return data().size();
  }

  inline auto IsEmpty() const -> bool {
    return data().empty();
  }

  auto Contains(Object* rhs) const -> bool {
    const auto& pos = Find(rhs);
    return pos != std::end(data());
  }

  DECLARE_TYPE(Set);

 public:
  static auto Of(Object* value) -> Set*;
  static inline auto Of(const StorageType& data = {}) -> Set* {
    return new Set(data);
  }

  static inline auto Empty() -> Set* {
    return Of();
  }
};

class Map : public Object {
 public:
  using StorageType = std::unordered_map<Object*, Object*, ObjectHasher, ObjectComparator>;
  using Iter = StorageType::iterator;
  using ConstIter = StorageType::const_iterator;

 private:
  StorageType data_;

  explicit Map(const StorageType& data) :
    Object(),
    data_(data) {}

  inline auto Find(Object* rhs) const -> ConstIter {
    return data().find(rhs);
  }

 public:
  ~Map() override = default;

  auto data() const -> const StorageType& {
    return data_;
  }

  auto GetSize() const -> uword {
    return data_.size();
  }

  auto IsEmpty() const -> bool {
    return data_.empty();
  }

  auto Contains(Object* rhs) const -> bool {
    return Find(rhs) != std::end(data());
  }

  auto Get(Object* key) const -> Object*;
  DECLARE_TYPE(Map);

 public:
  static inline auto New(const StorageType& data = {}) -> Map* {
    return new Map(data);
  }
};

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

static inline auto Null() -> Object* {
  return Pair::Empty();
}

static inline auto IsNull(Object* rhs) -> bool {
  return !rhs || (rhs->IsPair() && rhs->AsPair()->IsEmpty());
}

static inline auto BinaryAnd(Object* lhs, Object* rhs) -> Object* {
  ASSERT(lhs);
  ASSERT(rhs);
  return lhs->And(rhs);
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
