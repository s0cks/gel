#include "scheme/type.h"

#include <glog/logging.h>

#include <sstream>
#include <utility>

#include "scheme/common.h"

namespace scm {
void Type::Init() {
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

static Bool* kTrue = nullptr;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static Bool* kFalse = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

auto Bool::Equals(Type* rhs) const -> bool {
  if (!rhs->IsBool())
    return false;
  return Get() == rhs->AsBool()->Get();
}

auto Bool::ToString() const -> std::string {
  return Get() ? "#T" : "#F";
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

auto Number::Equals(Type* rhs) const -> bool {
  if (!rhs->IsNumber())
    return false;
  const auto other = rhs->AsNumber();
  return GetValue() == other->GetValue();
}

auto Number::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Number(";
  ss << "value=" << GetValue();
  ss << ")";
  return ss.str();
}

auto Number::Add(Datum* rhs) const -> Datum* {
  if (rhs->IsNumber()) {
    return Number::New(GetValue() + rhs->AsNumber()->GetValue());
  }
  LOG(ERROR) << this << " + " << rhs << " is invalid!";
  return Null::Get();
}

auto Number::Sub(Datum* rhs) const -> Datum* {
  if (rhs->IsNumber()) {
    return Number::New(GetValue() - rhs->AsNumber()->GetValue());
  }
  LOG(ERROR) << this << " - " << rhs << " is invalid!";
  return Null::Get();
}

auto Number::Mul(Datum* rhs) const -> Datum* {
  if (rhs->IsNumber()) {
    return Number::New(GetValue() * rhs->AsNumber()->GetValue());
  }
  LOG(ERROR) << this << " * " << rhs << " is invalid!";
  return Null::Get();
}

auto Number::Div(Datum* rhs) const -> Datum* {
  if (rhs->IsNumber()) {
    return Number::New(GetValue() / rhs->AsNumber()->GetValue());
  }
  LOG(ERROR) << this << " / " << rhs << " is invalid!";
  return Null::Get();
}

auto Number::Mod(Datum* rhs) const -> Datum* {
  if (rhs->IsNumber())
    return Number::New(GetValue() % rhs->AsNumber()->GetValue());
  LOG(ERROR) << this << " % " << rhs << " is invalid!";
  return Null::Get();
}

auto Pair::Equals(Type* rhs) const -> bool {
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

auto Symbol::Equals(Type* rhs) const -> bool {
  if (!rhs->IsSymbol())
    return false;
  const auto other = rhs->AsSymbol();
  return Get() == other->Get();
}

auto Symbol::Equals(const std::string& rhs) const -> bool {
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

auto Null::Equals(Type* rhs) const -> bool {
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

auto String::Equals(Type* rhs) const -> bool {
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
}  // namespace scm