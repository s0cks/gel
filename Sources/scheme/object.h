#ifndef SCM_TYPE_H
#define SCM_TYPE_H

#include <fmt/format.h>

#include <functional>
#include <ostream>
#include <string>
#include <unordered_set>

#include "scheme/common.h"

namespace scm {
#define FOR_EACH_TYPE(V) \
  V(Bool)                \
  V(Lambda)              \
  V(Procedure)           \
  V(NativeProcedure)     \
  V(Null)                \
  V(Pair)                \
  V(List)                \
  V(Number)              \
  V(Double)              \
  V(Long)                \
  V(String)              \
  V(Symbol)              \
  V(Macro)               \
  V(CompiledExpression)  \
  V(Error)

#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class Datum;
class Object {
  DEFINE_NON_COPYABLE_TYPE(Object)
 protected:
  Object() = default;

 public:
  virtual ~Object() = default;
  virtual auto GetObjectname() const -> const char* = 0;
  virtual auto Equals(Object* rhs) const -> bool = 0;
  virtual auto ToString() const -> std::string = 0;

  virtual auto AsDatum() -> Datum* {
    return nullptr;
  }

  auto IsDatum() -> bool {
    return AsDatum() != nullptr;
  }

  virtual auto IsAtom() const -> bool {
    return false;
  }

#define DEFINE_TYPE_CHECK(Name)      \
  virtual auto As##Name() -> Name* { \
    return nullptr;                  \
  }                                  \
  auto Is##Name() -> bool {          \
    return As##Name() != nullptr;    \
  }
  FOR_EACH_TYPE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
 public:
  static void Init();
};

static inline auto operator<<(std::ostream& stream, Object* rhs) -> std::ostream& {
  return stream << rhs->ToString();
}

#define DECLARE_TYPE(Name)                             \
  DEFINE_NON_COPYABLE_TYPE(Name)                       \
 public:                                               \
  ~Name() override = default;                          \
  auto GetObjectname() const -> const char* override { \
    return #Name;                                      \
  }                                                    \
  auto Equals(Object* rhs) const -> bool override;     \
  auto ToString() const -> std::string override;       \
  auto As##Name() -> Name* override {                  \
    return this;                                       \
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

class Null : public Datum {
 protected:
  Null() = default;

 public:
  auto IsAtom() const -> bool override {
    return false;
  }

  auto Add(Datum* rhs) const -> Datum* override {
    if (rhs->IsAtom())
      return rhs;
    return Get();
  }

  auto Sub(Datum* rhs) const -> Datum* override {
    if (rhs->IsAtom())
      return rhs;
    return Get();
  }

  auto Mul(Datum* rhs) const -> Datum* override {
    if (rhs->IsAtom())
      return rhs;
    return Get();
  }

  auto Div(Datum* rhs) const -> Datum* override {
    if (rhs->IsAtom())
      return rhs;
    return Get();
  }

  DECLARE_TYPE(Null);

 public:
  static auto Get() -> Null*;
  static inline auto New() -> Null* {
    return new Null();
  }
};

class Bool : public Datum {
 private:
  bool value_;

  explicit Bool(const bool value) :
    Datum(),
    value_(value) {}

 public:
  auto Get() const -> bool {
    return value_;
  }

  auto And(Datum* rhs) const -> Datum* override;
  auto Or(Datum* rhs) const -> Datum* override;

  DECLARE_TYPE(Bool);

 public:
  static void Init();
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
  DEFINE_NON_COPYABLE_TYPE(Number);

 private:
  union {
    uint64_t long_;
    double double_;
  };

 protected:
  explicit Number(const uint64_t value) :
    Datum(),
    long_(value) {}
  explicit Number(const double value) :
    Datum(),
    double_(value) {}

 public:
  ~Number() override = default;

  auto GetLong() const -> uint64_t {
    return long_;
  }

  auto GetDouble() const -> double {
    return double_;
  }

  auto AsNumber() -> Number* override {
    return this;
  }

 public:
  static auto New(const uint64_t rhs) -> Number*;
  static auto New(const double rhs) -> Number*;
};

class Long : public Number {
 protected:
  explicit Long(const uint64_t value) :
    Number(value) {}

 public:
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
};

class Double : public Number {
 protected:
  Double(const double value) :
    Number(value) {}

 public:
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

 public:
  auto GetCar() const -> Object* {
    return car_;
  }

  inline auto HasCar() const -> bool {
    return GetCar() != nullptr;
  }

  void SetCar(Datum* rhs) {
    ASSERT(rhs);
    car_ = rhs;
  }

  auto GetCdr() const -> Object* {
    return cdr_;
  }

  inline auto HasCdr() const -> bool {
    return GetCdr() != nullptr;
  }

  void SetCdr(Datum* rhs) {
    ASSERT(rhs);
    cdr_ = rhs;
  }

  DECLARE_TYPE(Pair);

 public:
  static inline auto New(Object* car = nullptr, Object* cdr = nullptr) -> Pair* {
    return new Pair(car, cdr);
  }
};

class StringObject : public Datum {
  DEFINE_NON_COPYABLE_TYPE(StringObject);

 private:
  std::string value_;

 protected:
  explicit StringObject(const std::string& value) :
    Datum(),
    value_(value) {}

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

static inline auto IsString(Object* rhs) -> bool {
  return rhs && rhs->IsString();
}

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
  DECLARE_TYPE(Symbol);

 public:
  static auto New(const std::string& rhs) -> Symbol*;
};

static inline auto IsSymbol(Object* rhs) -> bool {
  return rhs && rhs->IsSymbol();
}

using SymbolList = std::vector<Symbol*>;
using SymbolSet = std::unordered_set<Symbol*, Symbol::Comparator>;

static inline auto operator<<(std::ostream& stream, const SymbolList& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& symbol : rhs) {
    stream << symbol;
    if (--remaining >= 1)
      stream << ", ";
  }
  stream << "]";
  return stream;
}

class List : public Datum {
 private:
  std::vector<Datum*> data_;

  explicit List(const std::vector<Datum*>& data) :
    data_(data) {}

 public:
  auto GetLength() const -> uint64_t {
    return data_.size();
  }

  auto GetDatumAt(const uint64_t idx) const -> Datum* {
    ASSERT(idx >= 0 && idx <= GetLength());
    return data_[idx] ? data_[idx] : Null::Get();
  }

  void SetDatumAt(const uint64_t idx, Datum* rhs) {
    ASSERT(idx >= 0 && idx <= GetLength());
    data_[idx] = rhs ? rhs : Null::Get();
  }

  DECLARE_TYPE(List);

 public:
  static inline auto New(const std::vector<Datum*>& data = {}) -> List* {
    return new List(data);
  }
};

auto PrintValue(std::ostream& stream, Object* value) -> std::ostream&;

static inline auto BinaryAnd(Object* lhs, Object* rhs) -> Datum* {
  ASSERT(lhs && lhs->IsDatum());
  ASSERT(rhs && rhs->IsDatum());
  return lhs->AsDatum()->And(rhs->AsDatum());
}

static inline auto Car(Object* rhs) -> Object* {
  ASSERT(rhs);
  if (rhs->IsPair())
    return rhs->AsPair()->GetCar();
  return Null::Get();
}

static inline auto Cdr(Object* rhs) -> Object* {
  ASSERT(rhs);
  if (rhs->IsPair())
    return rhs->AsPair()->GetCdr();
  LOG(ERROR) << rhs << " is not a Pair or List.";
  return Null::Get();
}

static inline auto Truth(scm::Object* rhs) -> bool {
  ASSERT(rhs);
  if (rhs->IsBool())
    return rhs->AsBool()->Get();
  return !rhs->IsNull();  // TODO: better truth?
}

static inline auto Not(Object* rhs) -> Object* {
  ASSERT(rhs);
  return Truth(rhs) ? Bool::False() : Bool::True();
}
}  // namespace scm

template <>
struct fmt::formatter<scm::Object> : public fmt::formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const scm::Object& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}", value.ToString());
  }
};

#endif  // SCM_TYPE_H
