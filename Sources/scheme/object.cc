#include "scheme/object.h"

#include <glog/logging.h>

#include <sstream>
#include <string>
#include <utility>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/native_procedure.h"

namespace scm {
void Object::Init() {
  DVLOG(10) << "initializing type system....";
  Class::Init();
  Bool::Init();
  Long::Init();
  Error::Init();
  Pair::Init();
  Double::Init();
  Symbol::Init();
  String::Init();
}

void Class::Init() {}

Class* Long::kClass = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void Long::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New("Long");
  ASSERT(kClass);
}

Class* Double::kClass = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void Double::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New("Double");
  ASSERT(kClass);
}

auto Class::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Class(";
  ss << "name=" << GetName()->Get();
  ss << ")";
  return ss.str();
}

auto Datum::Add(Datum* rhs) const -> Datum* {
  return Pair::Empty();
}

auto Datum::Sub(Datum* rhs) const -> Datum* {
  return Pair::Empty();
}

auto Datum::Mul(Datum* rhs) const -> Datum* {
  return Pair::Empty();
}

auto Datum::Div(Datum* rhs) const -> Datum* {
  return Pair::Empty();
}

auto Datum::Mod(Datum* rhs) const -> Datum* {
  return Pair::Empty();
}

auto Datum::And(Datum* rhs) const -> Datum* {
  return Pair::Empty();
}

auto Datum::Or(Datum* rhs) const -> Datum* {
  return Pair::Empty();
}

auto Datum::Compare(Datum* rhs) const -> int {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

static Bool* kTrue = nullptr;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static Bool* kFalse = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

auto Bool::Equals(Object* rhs) const -> bool {
  if (!rhs->IsBool())
    return false;
  return Get() == rhs->AsBool()->Get();
}

auto Bool::ToString() const -> std::string {
  return Get() ? "#T" : "#F";
}

auto Bool::And(Datum* rhs) const -> Datum* {
  return Box(Get() && Truth(rhs));
}

auto Bool::Or(Datum* rhs) const -> Datum* {
  return Box(Get() || Truth(rhs));
}

Class* Bool::kClass = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void Bool::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New("Bool");
  ASSERT(kClass);
  kTrue = NewTrue();
  kFalse = NewFalse();
}

auto Bool::New(const bool value) -> Bool* {
  return new Bool(value);
}

auto Bool::True() -> Bool* {
  ASSERT(kTrue);
  return kTrue;
}

auto Bool::False() -> Bool* {
  ASSERT(kFalse);
  return kFalse;
}

auto Number::New(const uint64_t rhs) -> Number* {
  return Long::New(rhs);
}

auto Number::New(const double rhs) -> Number* {
  return Double::New(rhs);
}

#define FOR_EACH_NUMBER_BINARY_OP(V) \
  V(Add, +)                          \
  V(Sub, -)                          \
  V(Mul, *)                          \
  V(Div, /)

#define DEFINE_BINARY_OP(Name, Op)                                                                                 \
  auto Long::Name(Datum* rhs) const -> Datum* {                                                                    \
    if (!rhs || !rhs->IsNumber()) {                                                                                \
      LOG(ERROR) << rhs << " is not a Number.";                                                                    \
      return Pair::Empty();                                                                                        \
    }                                                                                                              \
    const auto left_num = rhs->AsNumber();                                                                         \
    ASSERT(left_num);                                                                                              \
    const auto left_val = left_num->IsLong() ? left_num->GetLong() : static_cast<uint64_t>(left_num->GetDouble()); \
    return Long::New(Get() Op left_val);                                                                           \
  }
FOR_EACH_NUMBER_BINARY_OP(DEFINE_BINARY_OP);
DEFINE_BINARY_OP(Mod, %);
#undef DEFINE_BINARY_OP

auto Long::Compare(Datum* rhs) const -> int {
  ASSERT(rhs && rhs->IsLong());
  if (Get() < rhs->AsLong()->Get())
    return -1;
  else if (Get() > rhs->AsLong()->Get())
    return +1;
  ASSERT(Get() == rhs->AsLong()->Get());
  return 0;
}

auto Long::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsLong())
    return false;
  const auto other = rhs->AsLong();
  return Get() == other->Get();
}

auto Long::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Number(";
  ss << "value=" << Get();
  ss << ")";
  return ss.str();
}

#define DEFINE_BINARY_OP(Name, Op)                                                          \
  auto Double::Name(Datum* rhs) const -> Datum* {                                           \
    if (!rhs || !rhs->IsNumber()) {                                                         \
      LOG(ERROR) << rhs << " is not a Number.";                                             \
      return Pair::Empty();                                                                 \
    }                                                                                       \
    const auto left_num = rhs->AsNumber();                                                  \
    ASSERT(left_num);                                                                       \
    const auto left_val = left_num->IsLong() ? left_num->GetLong() : left_num->GetDouble(); \
    return Double::New(Get() Op left_val);                                                  \
  }
FOR_EACH_NUMBER_BINARY_OP(DEFINE_BINARY_OP);
#undef DEFINE_BINARY_OP

auto Double::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsDouble())
    return false;
  return Get() == rhs->AsDouble()->Get();
}

auto Double::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Double(";
  ss << "value=" << Get();
  ss << ")";
  return ss.str();
}

Class* Pair::kClass = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void Pair::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New("Pair");
  ASSERT(kClass);
}

auto Pair::Equals(Object* rhs) const -> bool {
  if (!rhs->IsPair())
    return false;
  const auto other = rhs->AsPair();
  return GetCar()->Equals(other->GetCar()) && GetCdr()->Equals(other->GetCdr());
}

auto Pair::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Pair(";
  if (HasCar())
    ss << "car=" << GetCar()->ToString();
  if (HasCdr()) {
    if (HasCar())
      ss << ",";
    ss << "cdr=" << GetCdr()->ToString();
  }
  ss << ")";
  return ss.str();
}

auto Pair::Empty() -> Pair* {
  return new Pair();
}

auto Symbol::Equals(Object* rhs) const -> bool {
  if (!rhs->IsSymbol())
    return false;
  const auto other = rhs->AsSymbol();
  return Get() == other->Get();
}

auto StringObject::Equals(const std::string& rhs) const -> bool {
  return Get().compare(rhs) == 0;
}

Class* Symbol::kClass = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void Symbol::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New("Symbol");
  ASSERT(kClass);
}

auto Symbol::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Symbol(";
  ss << "value=" << Get();
  ss << ")";
  return ss.str();
}

auto Symbol::New(const std::string& rhs) -> Symbol* {
  ASSERT(!rhs.empty());
  return new Symbol(rhs);
}

Class* String::kClass = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void String::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New("String");
  ASSERT(kClass);
}

auto String::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  if (!rhs->IsString())
    return false;
  const auto other = rhs->AsString();
  ASSERT(other);
  return Get() == other->Get();
}

auto String::ToString() const -> std::string {
  std::stringstream ss;
  ss << "String(";
  ss << "value=" << Get();
  ss << ")";
  return ss.str();
}

auto String::ValueOf(Object* rhs) -> String* {
  std::stringstream ss;
  if (rhs->IsBool()) {
    ss << (Bool::Unbox(rhs->AsBool()) ? "#t" : "#f");
  } else if (rhs->IsLong()) {
    ss << rhs->AsLong()->Get();
  } else if (rhs->IsDouble()) {
    ss << rhs->AsDouble()->Get();
  } else if (rhs->IsSymbol()) {
    ss << rhs->AsSymbol()->Get();
  } else {
    ss << rhs->ToString();
  }
  return String::New(ss.str());
}

auto PrintValue(std::ostream& stream, Object* value) -> std::ostream& {
  ASSERT(value);
  if (value->IsBool()) {
    return stream << (value->AsBool()->Get() ? "#t" : "#f");
  } else if (value->IsDouble()) {
    return stream << (value->AsDouble()->Get());
  } else if (value->IsLong()) {
    return stream << (value->AsLong())->Get();
  } else if (value->IsString()) {
    return stream << '"' << value->AsString()->Get() << '"';
  } else if (value->IsSymbol()) {
    return stream << value->AsSymbol()->Get();
  } else if (value->IsNativeProcedure()) {
    stream << "NativeProcedure #" << value->AsNativeProcedure()->GetSymbol()->Get();
    return stream;
  } else if (value->IsPair()) {
    stream << "(";
    if (!value->AsPair()->IsEmpty()) {
      PrintValue(stream, value->AsPair()->GetCar()) << ", ";
      PrintValue(stream, value->AsPair()->GetCdr());
    }
    stream << ")";
    return stream;
  }
  return stream << value->ToString();
}

auto Class::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsClass())
    return false;
  const auto other = rhs->AsClass();
  ASSERT(other);
  return GetName()->Equals(other->GetName());
}

auto Class::New(const std::string& name) -> Class* {
  ASSERT(!name.empty());
  return New(String::New(name));
}
}  // namespace scm