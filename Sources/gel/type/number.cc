#include "gel/number.h"

#include "gel/exception.h"
#include "gel/to_string_helper.h"

namespace gel {

auto Number::CreateClass() -> Class* {
  return Class::New(Class::kNumberClassId, Number::GetClass(), kClassName);
}

auto Number::New(const ObjectList& args) -> Number* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Number::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= Get();
  return hash;
}

auto Number::Compare(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsNumber())
    return false;
  return Get() < rhs->AsNumber()->Get();
}

auto Number::BitNot() const -> Object* {
  return Number::New(~AsRaw<uint64_t>());
}

auto Number::ToString() const -> std::string {
  return ToStringHelper<Number>{};
}

// #define FOR_EACH_NUMBER_BINARY_OP(V) \
//   V(Add, +)                          \
//   V(Subtract, -)                     \
//   V(Multiply, *)                     \
//   V(Divide, /)

// #define DEFINE_BINARY_OP(Name, Op)                                                                                 \
//   auto Number::Name(Object* rhs) const -> Object* {                                                                  \
//     if (!rhs || !rhs->IsNumber()) {                                                                                \
//       LOG(ERROR) << rhs << " is not a Number.";                                                                    \
//       return Pair::Empty();                                                                                        \
//     }                                                                                                              \
//     const auto left_num = rhs->AsNumber();                                                                         \
//     ASSERT(left_num);                                                                                              \
//     const auto left_val = left_num->IsNumber() ? left_num->GetNumber() : static_cast<uint64_t>(left_num->GetDouble()); \
//     return Number::New(Get() Op left_val);                                                                           \
//   }
// FOR_EACH_NUMBER_BINARY_OP(DEFINE_BINARY_OP);
// DEFINE_BINARY_OP(Modulus, %);
// #undef DEFINE_BINARY_OP

// auto Number::Compare(Object* rhs) const -> bool {
//   if (!rhs || !rhs->IsNumber())
//     return false;
//   return Get() < rhs->AsNumber()->Get();
// }

// auto Number::Equals(Object* rhs) const -> bool {
//   if (!rhs || !rhs->IsNumber())
//     return false;
//   const auto other = rhs->AsNumber();
//   return Get() == other->Get();
// }

// auto Number::Eq(Object* rhs) const -> Object* {
//   return Bool::Box(Equals(rhs));
// }

// auto Number::BitAnd(Object* rhs) const -> Object* {
//   if (rhs->IsNil())
//     throw IllegalArgumentException("rhs equals '()");
//   else if (!rhs->IsNumber())
//     throw IllegalArgumentException(fmt::format("expected `{}` to be a Number.", (*rhs)));
//   return Number::New(GetNumber() & rhs->AsNumber()->GetNumber());
// }

// auto Number::BitOr(Object* rhs) const -> Object* {
//   if (rhs->IsNil())
//     throw IllegalArgumentException("rhs equals '()");
//   else if (!rhs->IsNumber())
//     throw IllegalArgumentException(fmt::format("expected `{}` to be a Number.", (*rhs)));
//   return Number::New(GetNumber() | rhs->AsNumber()->GetNumber());
// }

// auto Number::BitXor(Object* rhs) const -> Object* {
//   ASSERT(rhs);
//   if (!rhs->IsNumber())
//     throw Exception(fmt::format("{} is not a Number.", (*rhs)));
//   return Number::New(GetNumber() ^ rhs->AsNumber()->GetNumber());
// }

// auto Number::ShiftLeft(Object* rhs) const -> Object* {
//   ASSERT(rhs);
//   if (!rhs->IsNumber())
//     throw Exception(fmt::format("{} is not a Number.", (*rhs)));
//   return Number::New(GetNumber() << rhs->AsNumber()->GetNumber());
// }

// auto Number::ShiftRight(Object* rhs) const -> Object* {
//   ASSERT(rhs);
//   if (!rhs->IsNumber())
//     throw Exception(fmt::format("{} is not a Number.", (*rhs)));
//   return Number::New(GetNumber() >> rhs->AsNumber()->GetNumber());
// }

// auto Number::GreaterThan(Object* rhs) const -> Object* {
//   if (!rhs || !rhs->IsNumber())
//     return Bool::False();
//   return Bool::Box(Get() > rhs->AsNumber()->GetNumber());
// }

// auto Number::LessThan(Object* rhs) const -> Object* {
//   if (!rhs || !rhs->IsNumber())
//     return Bool::False();
//   return Bool::Box(Get() < rhs->AsNumber()->GetNumber());
// }

// auto Number::ToString() const -> std::string {
//   ToStringHelper<Number> helper;
//   helper.AddField("value", Get());
//   return helper;
// }

// #define DEFINE_BINARY_OP(Name, Op)                                                              \
//   auto Double::Name(Object* rhs) const -> Object* {                                             \
//     if (!rhs || !rhs->IsNumber()) {                                                             \
//       LOG(ERROR) << rhs << " is not a Number.";                                                 \
//       return Pair::Empty();                                                                     \
//     }                                                                                           \
//     const auto left_num = rhs->AsNumber();                                                      \
//     ASSERT(left_num);                                                                           \
//     const auto left_val = left_num->IsDouble() ? left_num->GetDouble() : left_num->GetDouble(); \
//     return Double::New(Get() Op left_val);                                                      \
//   }
// FOR_EACH_NUMBER_BINARY_OP(DEFINE_BINARY_OP)
// #undef DEFINE_BINARY_OP

// auto Double::Equals(Object* rhs) const -> bool {
//   if (!rhs || !rhs->IsDouble())
//     return false;
//   return Get() == rhs->AsDouble()->Get();
// }

// auto Double::ToString() const -> std::string {
//   ToStringHelper<Double> helper;
//   helper.AddField("value", Get());
//   return helper;
// }

}  // namespace gel