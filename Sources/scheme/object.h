#ifndef SCM_OBJECT_H
#define SCM_OBJECT_H

#include <fmt/format.h>

#include <functional>
#include <ostream>
#include <rpp/observables/dynamic_observable.hpp>
#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/observers/observer.hpp>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "gmock/gmock.h"
#include "scheme/common.h"
#include "scheme/pointer.h"
#include "scheme/rx.h"
#include "scheme/type.h"

namespace scm {
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

namespace instr {
class GraphEntryInstr;
}

class Executable {
  DEFINE_NON_COPYABLE_TYPE(Executable);

 private:
  instr::GraphEntryInstr* entry_ = nullptr;

 protected:
  Executable() = default;

  void SetEntry(instr::GraphEntryInstr* entry) {
    ASSERT(entry);
    entry_ = entry;
  }

 public:
  virtual ~Executable() = default;

  auto GetEntry() const -> instr::GraphEntryInstr* {
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

class Class;
using ClassList = std::vector<Class*>;

class Class : public Datum {
  friend class Object;

 private:
  Class* parent_;
  String* name_;

 protected:
  explicit Class(Class* parent, String* name) :
    Datum(),
    parent_(parent),
    name_(name) {
    ASSERT(name_);
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;

 public:
  ~Class() override = default;

  auto GetParent() const -> Class* {
    return parent_;
  }

  inline auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  auto GetName() const -> String* {
    return name_;
  }

  auto IsInstanceOf(Class* rhs) const -> bool;
  DECLARE_TYPE(Class);

 private:
  static ClassList all_;

  static inline auto New(String* name) -> Class* {
    ASSERT(name);
    return new Class(nullptr, name);
  }

  static auto New(const std::string& name) -> Class*;

 public:
  static auto New(Class* parent, String* name) -> Class*;
  static auto New(Class* parent, const std::string& name) -> Class*;

  static inline auto GetAllClasses() -> const ClassList& {
    return all_;
  }

  static auto FindClass(String* name) -> Class*;
  static auto FindClass(Symbol* name) -> Class*;
};

class ClassListIterator {
  DEFINE_NON_COPYABLE_TYPE(ClassListIterator);

 private:
  const ClassList& values_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  ClassList::const_iterator iter_;

 public:
  explicit ClassListIterator(const ClassList& values = Class::GetAllClasses()) :
    values_(values),
    iter_(std::begin(values)) {}
  ~ClassListIterator() = default;

  auto values() const -> const ClassList& {
    return values_;
  }

  auto iter() const -> const ClassList::const_iterator& {
    return iter_;
  }

  auto HasNext() const -> bool {
    return iter() != std::end(values());
  }

  auto Next() -> Class* {
    const auto next = (*iter_);
    ASSERT(next);
    iter_++;
    return next;
  }
};

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

  auto Equals(const std::string& rhs) const -> bool;
};

class String : public StringObject {
 protected:
  explicit String(const std::string& value) :
    StringObject(value) {}

 public:
  ~String() override = default;

  DECLARE_TYPE(String);

 public:
  static inline auto New(const std::string& value) -> String* {
    return new String(value);
  }

  static inline auto Unbox(Object* rhs) -> const std::string& {
    ASSERT(rhs && rhs->IsString());
    return rhs->AsString()->Get();
  }

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

#ifdef SCM_ENABLE_RX

class Observer : public Object {
 private:
  rx::DynamicObjectObserver value_;

 protected:
  explicit Observer(rx::DynamicObjectObserver value) :
    value_(value) {}

 public:
  ~Observer() override = default;

  auto Get() const -> const rx::DynamicObjectObserver& {
    return value_;
  }

  DECLARE_TYPE(Observer);

 private:
  static auto CreateDynamicObserver(Procedure* on_next, Procedure* on_error, Procedure* on_completed)
      -> rx::DynamicObjectObserver;

 public:
  static inline auto New(Procedure* on_next, Procedure* on_error, Procedure* on_completed) -> Observer* {
    ASSERT(on_next);
    return new Observer(CreateDynamicObserver(on_next, on_error, on_completed));
  }
};

class Observable : public Object {
  friend class proc::rx_buffer;
  friend class proc::rx_map;
  friend class proc::rx_subscribe;

 private:
  rx::DynamicObjectObservable value_;

 private:
  explicit Observable(const rx::DynamicObjectObservable& value) :
    value_(value) {}

 public:
  ~Observable() override = default;

  auto GetValue() const -> const rx::DynamicObjectObservable& {
    return value_;
  }

  template <typename O>
  void Apply(O&& op) {
    value_ = value_ | std::forward<O>(op);
  }

  template <typename S>
  void Subscribe(S&& on_next) {
    value_.subscribe(std::forward<S>(on_next));
  }

  DECLARE_TYPE(Observable);

 public:
  static inline auto New(const rx::DynamicObjectObservable& value) -> Observable* {
    return new Observable(value);
  }

  static auto New(Object* value) -> Observable*;
  static auto Empty() -> Observable*;

 public:  //???
  static auto ToObservable(Pair* list) -> rx::DynamicObjectObservable;
};

#endif  // SCM_ENABLE_RX

auto PrintValue(std::ostream& stream, Object* value) -> std::ostream&;

#define DEFINE_TYPE_PRED(Name)                     \
  static inline auto Is##Name(Object* rhs)->bool { \
    return rhs && rhs->Is##Name();                 \
  }
DEFINE_TYPE_PRED(Array);
FOR_EACH_TYPE(DEFINE_TYPE_PRED)
#undef DEFINE_TYPE_PRED

#define DEFINE_TYPE_CAST(Name)                      \
  static inline auto To##Name(Object* rhs)->Name* { \
    ASSERT(rhs&& Is##Name(rhs));                    \
    return rhs->As##Name();                         \
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

static inline auto Truth(scm::Object* rhs) -> bool {
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
struct has_to_string {
  static const bool value = false;
};

#define DECLARE_HAS_TO_STRING(Name) \
  template <>                       \
  struct has_to_string<Name> {      \
    static const bool value = true; \
  };

DECLARE_HAS_TO_STRING(Object);
FOR_EACH_TYPE(DECLARE_HAS_TO_STRING)
#undef DECLARE_HAS_TO_STRING

template <typename T>
static inline auto Stringify(std::ostream& stream, const std::vector<T*>& values,
                             std::enable_if_t<scm::has_to_string<T>::value>* = nullptr) -> std::ostream& {
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
}  // namespace scm

namespace fmt {
template <>
struct formatter<scm::Symbol> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const scm::Symbol& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.Get());
  }
};
}  // namespace fmt

template <>
struct fmt::formatter<scm::Object> : public fmt::formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const scm::Object& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}", value.ToString());
  }
};

#endif  // SCM_OBJECT_H
