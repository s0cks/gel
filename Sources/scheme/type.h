#ifndef SCM_TYPE_H
#define SCM_TYPE_H

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
  V(Null)                \
  V(Pair)                \
  V(List)                \
  V(Number)              \
  V(String)              \
  V(Symbol)              \
  V(Macro)

#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class Datum;
class Type {
  DEFINE_NON_COPYABLE_TYPE(Type)
 protected:
  Type() = default;

 public:
  virtual ~Type() = default;
  virtual auto GetTypename() const -> const char* = 0;
  virtual auto Equals(Type* rhs) const -> bool = 0;
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

static inline auto operator<<(std::ostream& stream, Type* rhs) -> std::ostream& {
  return stream << rhs->ToString();
}

#define DECLARE_TYPE(Name)                           \
  DEFINE_NON_COPYABLE_TYPE(Name)                     \
 public:                                             \
  ~Name() override = default;                        \
  auto GetTypename() const -> const char* override { \
    return #Name;                                    \
  }                                                  \
  auto Equals(Type* rhs) const -> bool override;     \
  auto ToString() const -> std::string override;     \
  auto As##Name() -> Name* override {                \
    return this;                                     \
  }

class Datum : public Type {
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
};

class Number : public Datum {
 private:
  uintptr_t value_;

  explicit Number(const uintptr_t value) :
    Datum(),
    value_(value) {}

 public:
  auto GetValue() const -> uintptr_t {
    return value_;
  }

  auto Add(Datum* rhs) const -> Datum* override;
  auto Sub(Datum* rhs) const -> Datum* override;
  auto Mul(Datum* rhs) const -> Datum* override;
  auto Div(Datum* rhs) const -> Datum* override;
  auto Mod(Datum* rhs) const -> Datum* override;
  DECLARE_TYPE(Number);

 public:
  static inline auto New(const uintptr_t value) -> Number* {
    return new Number(value);
  }
};

class Pair : public Datum {
 private:
  Datum* car_;
  Datum* cdr_;

 protected:
  explicit Pair(Datum* car = nullptr, Datum* cdr = nullptr) :
    car_(car),
    cdr_(cdr) {}

 public:
  auto GetCar() const -> Datum* {
    return car_;
  }

  inline auto HasCar() const -> bool {
    return GetCar() != nullptr;
  }

  void SetCar(Datum* rhs) {
    ASSERT(rhs);
    car_ = rhs;
  }

  auto GetCdr() const -> Datum* {
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
  static inline auto New(Datum* car = nullptr, Datum* cdr = nullptr) -> Pair* {
    return new Pair(car, cdr);
  }
};

class Symbol : public Datum {
 public:
  struct Comparator {
    auto operator()(Symbol* lhs, Symbol* rhs) const -> bool {
      ASSERT(lhs && rhs);
      return lhs->Get() < rhs->Get();
    }
  };

 private:
  std::string value_;

  explicit Symbol(const std::string& value) :
    Datum(),
    value_(value) {}

 public:
  auto Get() const -> const std::string& {
    return value_;
  }

  auto Equals(const std::string& rhs) const -> bool;
  DECLARE_TYPE(Symbol);

 public:
  static auto New(const std::string& rhs) -> Symbol*;
};

class String : public Datum {
 private:
  std::string value_;

  explicit String(const std::string& value) :
    Datum(),
    value_(value) {}

 public:
  auto Get() const -> const std::string& {
    return value_;
  }

  DECLARE_TYPE(String);

 public:
  static inline auto New(const std::string& value) -> String* {
    return new String(value);
  }
};

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

static inline auto PrintValue(std::ostream& stream, Type* value) -> std::ostream& {
  ASSERT(value);
  if (value->IsNull()) {
    return stream << "`()";
  } else if (value->IsBool()) {
    return stream << (value->AsBool()->Get() ? "#t" : "#f");
  } else if (value->IsNumber()) {
    return stream << value->AsNumber()->GetValue();
  } else if (value->IsString()) {
    return stream << '"' << value->AsString()->Get() << '"';
  }
  return stream << value->ToString();
}
}  // namespace scm

#endif  // SCM_TYPE_H
