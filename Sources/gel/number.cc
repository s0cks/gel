#include "gel/number.h"

#include "gel/exception.h"
#include "gel/to_string_helper.h"

namespace gel {

auto Long::CreateClass() -> Class* {
  return Class::New(Class::kLongClassId, Number::GetClass(), kClassName);
}

auto Long::New(const ObjectList& args) -> Long* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Long::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= Get();
  return hash;
}

auto Long::Unbox(Object* rhs) -> int64_t {
  if (!rhs)
    throw Exception(fmt::format("expected null to be a Long."));
  if (!rhs->IsLong())
    throw Exception(fmt::format("expected `{}` to be a Long.", *rhs));
  return rhs->AsLong()->Get();
}

auto Double::Compare(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsDouble())
    return false;
  return Get() < rhs->AsDouble()->Get();
}

auto Double::CreateClass() -> Class* {
  return Class::New(Number::GetClass(), kClassName);
}

auto Double::New(const ObjectList& args) -> Double* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Double::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= Get();
  return hash;
}

auto Number::BitNot() const -> Object* {
  return Long::New(~GetLong());
}

auto Number::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Number::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Number::New(const ObjectList& args) -> Number* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Number::New(const RawLong rhs) -> Number* {
  return Long::New(rhs);
}

auto Number::New(const double rhs) -> Number* {
  return Double::New(rhs);
}

auto Number::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return kInvalidHashCode;
}

auto Number::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsNumber())
    return false;
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Number::ToString() const -> std::string {
  return ToStringHelper<Number>{};
}

#define FOR_EACH_NUMBER_BINARY_OP(V) \
  V(Add, +)                          \
  V(Subtract, -)                     \
  V(Multiply, *)                     \
  V(Divide, /)

#define DEFINE_BINARY_OP(Name, Op)                                                                                 \
  auto Long::Name(Object* rhs) const -> Object* {                                                                  \
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
DEFINE_BINARY_OP(Modulus, %);
#undef DEFINE_BINARY_OP

auto Long::Compare(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsLong())
    return false;
  return Get() < rhs->AsLong()->Get();
}

auto Long::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsLong())
    return false;
  const auto other = rhs->AsLong();
  return Get() == other->Get();
}

auto Long::Eq(Object* rhs) const -> Object* {
  return Bool::Box(Equals(rhs));
}

auto Long::BitAnd(Object* rhs) const -> Object* {
  if (rhs->IsNil())
    throw IllegalArgumentException("rhs equals '()");
  else if (!rhs->IsNumber())
    throw IllegalArgumentException(fmt::format("expected `{}` to be a Number.", (*rhs)));
  return Long::New(GetLong() & rhs->AsNumber()->GetLong());
}

auto Long::BitOr(Object* rhs) const -> Object* {
  if (rhs->IsNil())
    throw IllegalArgumentException("rhs equals '()");
  else if (!rhs->IsNumber())
    throw IllegalArgumentException(fmt::format("expected `{}` to be a Number.", (*rhs)));
  return Long::New(GetLong() | rhs->AsNumber()->GetLong());
}

auto Long::BitXor(Object* rhs) const -> Object* {
  ASSERT(rhs);
  if (!rhs->IsNumber())
    throw Exception(fmt::format("{} is not a Number.", (*rhs)));
  return Long::New(GetLong() ^ rhs->AsNumber()->GetLong());
}

auto Long::ShiftLeft(Object* rhs) const -> Object* {
  ASSERT(rhs);
  if (!rhs->IsNumber())
    throw Exception(fmt::format("{} is not a Number.", (*rhs)));
  return Long::New(GetLong() << rhs->AsNumber()->GetLong());
}

auto Long::ShiftRight(Object* rhs) const -> Object* {
  ASSERT(rhs);
  if (!rhs->IsNumber())
    throw Exception(fmt::format("{} is not a Number.", (*rhs)));
  return Long::New(GetLong() >> rhs->AsNumber()->GetLong());
}

auto Long::GreaterThan(Object* rhs) const -> Object* {
  if (!rhs || !rhs->IsNumber())
    return Bool::False();
  return Bool::Box(Get() > rhs->AsNumber()->GetLong());
}

auto Long::LessThan(Object* rhs) const -> Object* {
  if (!rhs || !rhs->IsNumber())
    return Bool::False();
  return Bool::Box(Get() < rhs->AsNumber()->GetLong());
}

auto Long::ToString() const -> std::string {
  ToStringHelper<Long> helper;
  helper.AddField("value", Get());
  return helper;
}

#define DEFINE_BINARY_OP(Name, Op)                                                              \
  auto Double::Name(Object* rhs) const -> Object* {                                             \
    if (!rhs || !rhs->IsNumber()) {                                                             \
      LOG(ERROR) << rhs << " is not a Number.";                                                 \
      return Pair::Empty();                                                                     \
    }                                                                                           \
    const auto left_num = rhs->AsNumber();                                                      \
    ASSERT(left_num);                                                                           \
    const auto left_val = left_num->IsDouble() ? left_num->GetDouble() : left_num->GetDouble(); \
    return Double::New(Get() Op left_val);                                                      \
  }
FOR_EACH_NUMBER_BINARY_OP(DEFINE_BINARY_OP)
#undef DEFINE_BINARY_OP

auto Double::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsDouble())
    return false;
  return Get() == rhs->AsDouble()->Get();
}

auto Double::ToString() const -> std::string {
  ToStringHelper<Double> helper;
  helper.AddField("value", Get());
  return helper;
}

}  // namespace gel