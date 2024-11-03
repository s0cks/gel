#include "scheme/object.h"

#include <glog/logging.h>

#include <sstream>
#include <utility>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/native_procedure.h"

namespace scm {
void Object::Init() {
  DVLOG(10) << "initializing type system....";
  Bool::Init();
}

auto Datum::Add(Datum* rhs) const -> Datum* {
  return Null::Get();
}

auto Datum::Sub(Datum* rhs) const -> Datum* {
  return Null::Get();
}

auto Datum::Mul(Datum* rhs) const -> Datum* {
  return Null::Get();
}

auto Datum::Div(Datum* rhs) const -> Datum* {
  return Null::Get();
}

auto Datum::Mod(Datum* rhs) const -> Datum* {
  return Null::Get();
}

auto Datum::And(Datum* rhs) const -> Datum* {
  return Null::Get();
}

auto Datum::Or(Datum* rhs) const -> Datum* {
  return Null::Get();
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

void Bool::Init() {
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
      return Null::Get();                                                                                          \
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
      return Null::Get();                                                                   \
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

auto Symbol::Equals(Object* rhs) const -> bool {
  if (!rhs->IsSymbol())
    return false;
  const auto other = rhs->AsSymbol();
  return Get() == other->Get();
}

auto StringObject::Equals(const std::string& rhs) const -> bool {
  return Get().compare(rhs) == 0;
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

auto Null::Equals(Object* rhs) const -> bool {
  return rhs->IsNull();
}

auto Null::ToString() const -> std::string {
  return "()";
}

auto Null::Get() -> Null* {
  static Null* kNull = nullptr;  // NOLINT
  if (!kNull) {
    kNull = New();
    ASSERT(kNull);
  }
  return kNull;
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
  } else if (rhs->IsNull()) {
    ss << "'()";
  } else {
    ss << rhs->ToString();
  }
  return String::New(ss.str());
}

auto PrintValue(std::ostream& stream, Object* value) -> std::ostream& {
  ASSERT(value);
  if (value->IsNull()) {
    return stream << "`()";
  } else if (value->IsBool()) {
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
    PrintValue(stream, value->AsPair()->GetCar()) << ", ";
    PrintValue(stream, value->AsPair()->GetCdr());
    stream << ")";
    return stream;
  }
  return stream << value->ToString();
}
}  // namespace scm