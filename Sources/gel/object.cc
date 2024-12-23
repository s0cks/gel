#include "gel/object.h"

#include <glog/logging.h>

#include <exception>
#include <rpp/observers/fwd.hpp>
#include <rpp/observers/observer.hpp>
#include <rpp/sources/fwd.hpp>
#include <sstream>
#include <string>
#include <utility>

#include "gel/array.h"
#include "gel/common.h"
#include "gel/expression.h"
#include "gel/heap.h"
#include "gel/macro.h"
#include "gel/module.h"
#include "gel/namespace.h"
#include "gel/native_procedure.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/procedure.h"
#include "gel/runtime.h"
#include "gel/rx.h"
#include "gel/script.h"
#include "gel/to_string_helper.h"

namespace gel {
#ifdef GEL_DISABLE_HEAP

#define DEFINE_NEW_OPERATOR(Name)                     \
  auto Name::operator new(const size_t sz) -> void* { \
    return malloc(sz);                                \
  }

#else

#define DEFINE_NEW_OPERATOR(Name)                     \
  auto Name::operator new(const size_t sz) -> void* { \
    const auto heap = Heap::GetHeap();                \
    ASSERT(heap);                                     \
    const auto address = heap->TryAllocate(sz);       \
    ASSERT(address != UNALLOCATED);                   \
    return reinterpret_cast<void*>(address);          \
  }

#endif  // GEL_DISABLE_HEAP

FOR_EACH_TYPE(DEFINE_NEW_OPERATOR)  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
#undef DEFINE_NEW_OPERATOR

#define DEFINE_INIT_CLASS(Name)  \
  Class* Name::kClass = nullptr; \
  void Name::InitClass() {       \
    ASSERT(kClass == nullptr);   \
    kClass = CreateClass();      \
    ASSERT(kClass);              \
  }
DEFINE_INIT_CLASS(Object);
FOR_EACH_TYPE(DEFINE_INIT_CLASS)
#undef DEFINE_TYPE_INIT

auto Object::CreateClass() -> Class* {
  return Class::New(kClassName);
}

auto Object::raw_ptr() const -> Pointer* {
  const auto address = GetStartingAddress() - sizeof(Pointer);
  ASSERT(address >= UNALLOCATED);
  return Pointer::At(address);
}

auto Object::Add(Object* rhs) const -> Object* {
  NOT_IMPLEMENTED(ERROR);
  return Null();
}

auto Object::Sub(Object* rhs) const -> Object* {
  NOT_IMPLEMENTED(ERROR);
  return Null();
}

auto Object::Mul(Object* rhs) const -> Object* {
  NOT_IMPLEMENTED(ERROR);
  return Null();
}

auto Object::Div(Object* rhs) const -> Object* {
  NOT_IMPLEMENTED(ERROR);
  return Null();
}

auto Object::Mod(Object* rhs) const -> Object* {
  NOT_IMPLEMENTED(ERROR);
  return Null();
}

auto Object::And(Object* rhs) const -> Object* {
  NOT_IMPLEMENTED(ERROR);
  return Null();
}

auto Object::Or(Object* rhs) const -> Object* {
  NOT_IMPLEMENTED(ERROR);
  return Null();
}

auto Object::Compare(Object* rhs) const -> int {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

void Object::Init() {
  InitClass();
  Class::InitClass();
  Namespace::InitClass();
  Module::InitClass();
  Seq::InitClass();
  // exec
  Script::InitClass();
  Procedure::InitClass();
  Lambda::InitClass();
  NativeProcedure::Init();
  // types
  // numeric type(s)
  Number::InitClass();
  Long::InitClass();
  Double::InitClass();
  Pair::InitClass();
  Bool::Init();
  ArrayBase::InitClass();
  Macro::InitClass();
  // string-like type(s)
  String::InitClass();
  Symbol::InitClass();
  // error type(s)
  Error::InitClass();
  Set::InitClass();
#ifdef GEL_ENABLE_RX
  Observable::InitClass();
  Observer::InitClass();
  Subject::InitClass();
  ReplaySubject::InitClass();
  PublishSubject::InitClass();
#endif  // GEL_ENABLE_RX
}

auto Long::CreateClass() -> Class* {
  return Class::New(Number::GetClass(), kClassName);
}

auto Long::New(const ObjectList& args) -> Long* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Long::HashCode() const -> uword {
  uword hash = 0;
  CombineHash(hash, Get());
  return hash;
}

auto Long::Unbox(Object* rhs) -> uint64_t {
  if (!rhs || !rhs->IsLong())
    throw Exception(fmt::format("expected `{}` to be a Long.", *rhs));
  return rhs->AsLong()->Get();
}

auto Double::CreateClass() -> Class* {
  return Class::New(Number::GetClass(), kClassName);
}

auto Double::New(const ObjectList& args) -> Double* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Double::HashCode() const -> uword {
  uword hash = 0;
  CombineHash(hash, Get());
  return hash;
}

static Bool* kTrue = nullptr;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static Bool* kFalse = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

auto Bool::Equals(Object* rhs) const -> bool {
  if (!rhs->IsBool())
    return false;
  return Get() == rhs->AsBool()->Get();
}

auto Bool::New(const ObjectList& args) -> Bool* {
  if (args.empty())
    return False();
  else if (args.size() == 1)
    return Box(gel::Truth(args[0]));
  return Box(gel::Truth(gel::ToList(args)));
}

auto Bool::ToString() const -> std::string {
  return Get() ? "#T" : "#F";
}

auto Bool::And(Object* rhs) const -> Object* {
  return Box(Get() && Truth(rhs));
}

auto Bool::Or(Object* rhs) const -> Object* {
  return Box(Get() || Truth(rhs));
}

auto Bool::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

void Bool::Init() {
  InitClass();
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

auto Bool::HashCode() const -> uword {
  uword hash = 0;
  CombineHash(hash, Get());
  return hash;
}

auto Number::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Number::New(const ObjectList& args) -> Number* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Number::New(const uint64_t rhs) -> Number* {
  return Long::New(rhs);
}

auto Number::New(const double rhs) -> Number* {
  return Double::New(rhs);
}

auto Number::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
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
  V(Sub, -)                          \
  V(Mul, *)                          \
  V(Div, /)

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
DEFINE_BINARY_OP(Mod, %);
#undef DEFINE_BINARY_OP

auto Long::Compare(Object* rhs) const -> int {
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
  ToStringHelper<Long> helper;
  helper.AddField("value", Get());
  return helper;
}

#define DEFINE_BINARY_OP(Name, Op)                                                          \
  auto Double::Name(Object* rhs) const -> Object* {                                         \
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
  ToStringHelper<Double> helper;
  helper.AddField("value", Get());
  return helper;
}

auto Pair::New(const ObjectList& args) -> Pair* {
  if (args.empty())
    return Pair::Empty();
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Pair::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Pair::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Pair::Equals(Object* rhs) const -> bool {
  if (!rhs->IsPair())
    return false;
  const auto other = rhs->AsPair();
  return GetCar()->Equals(other->GetCar()) && GetCdr()->Equals(other->GetCdr());
}

auto Pair::ToString() const -> std::string {
  ToStringHelper<Pair> helper;
  helper.AddField("car", GetCar());
  helper.AddField("cdr", GetCdr());
  return helper;
}

static Pair* kEmptyPair = nullptr;
auto Pair::Empty() -> Pair* {
  if (kEmptyPair)
    return kEmptyPair;
  return kEmptyPair = Pair::NewEmpty();
}

auto Pair::HashCode() const -> uword {
  uword hash = 0;
  CombineHash(hash, HasCar() ? GetCar()->HashCode() : Null()->HashCode());
  CombineHash(hash, HasCdr() ? GetCdr()->HashCode() : Null()->HashCode());
  return hash;
}

auto Symbol::Equals(Object* rhs) const -> bool {
  return StringObject::Equals(rhs);
}

auto Symbol::HashCode() const -> uword {
  return StringObject::HashCode();
}

auto StringObject::Equals(Object* rhs) const -> bool {
  if (!rhs || !(rhs->IsString() || rhs->IsSymbol()))
    return false;
  return Get().compare(((StringObject*)rhs)->Get()) == 0;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

auto StringObject::Equals(const std::string& rhs) const -> bool {
  return Get().compare(rhs) == 0;
}

auto Symbol::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Symbol::New(const ObjectList& args) -> Symbol* {
  NOT_IMPLEMENTED(FATAL);
}

auto Symbol::ToString() const -> std::string {
  ToStringHelper<Symbol> helper;
  helper.AddField("value", Get());
  return helper;
}

auto Symbol::New(const std::string& rhs) -> Symbol* {
  ASSERT(!rhs.empty());
  return new Symbol(rhs);
}

auto String::New() -> String* {
  static auto kEmptyString = new String();
  ASSERT(kEmptyString);
  return kEmptyString;
}

auto String::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto String::Equals(Object* rhs) const -> bool {
  return StringObject::Equals(rhs);
}

auto String::New(const ObjectList& args) -> String* {
  if (args.empty() || gel::IsNull(args[0]))
    return New();
  if (args[0]->IsString())
    return String::New(args[0]->AsString()->Get());
  return ValueOf(args[0]);
}

auto String::ToString() const -> std::string {
  ToStringHelper<String> helper;
  helper.AddField("value", Get());
  return helper;
}

auto String::Empty() -> String* {
  static String* kEmpty = String::New();
  ASSERT(kEmpty);
  return kEmpty;
}

auto String::ValueOf(Object* rhs) -> String* {
  if (rhs->IsString())
    return rhs->AsString();
  else if (rhs->IsSymbol())
    return String::New(rhs->AsSymbol()->Get());
  std::stringstream ss;
  if (rhs->IsBool()) {
    ss << (Bool::Unbox(rhs->AsBool()) ? "#t" : "#f");
  } else if (rhs->IsLong()) {
    ss << rhs->AsLong()->Get();
  } else if (rhs->IsDouble()) {
    ss << rhs->AsDouble()->Get();
  } else if (rhs->IsSymbol()) {
    ss << rhs->AsSymbol()->Get();
  } else if (rhs->IsPair()) {
    const auto pair = rhs->AsPair();
    ASSERT(pair);
    ss << "(";
    if (pair->IsEmpty()) {
      ss << ")";
    } else {
      PrintValue(ss, pair->GetCar());
      auto next = pair->GetCdr();
      do {
        if (gel::IsNull(next)) {
          ss << ")";
          break;
        }
        if (!next->IsPair()) {
          ss << " ";
          PrintValue(ss, next);
          ss << ")";
          break;
        }
        ss << " ";
        PrintValue(ss, next->AsPair()->GetCar());
        next = next->AsPair()->GetCdr();
      } while (true);
    }
  } else {
    ss << rhs->ToString();
  }
  return String::New(ss.str());
}

auto String::HashCode() const -> uword {
  return StringObject::HashCode();
}

auto StringObject::HashCode() const -> uword {
  uword hash = 0;
  CombineHash(hash, Get());
  return hash;
}

auto Set::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

auto Set::Equals(Object* rhs) const -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Set::ToString() const -> std::string {
  ToStringHelper<Set> helper;
  helper.AddField("size", GetSize());
  return helper;
}

auto Set::New(const ObjectList& args) -> Set* {
  StorageType data(args.begin(), args.end());
  return Of(data);
}

auto Set::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Set");
}

auto Seq::New(const ObjectList& args) -> Seq* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return nullptr;
}

auto Seq::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Seq");
}

auto Seq::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

auto Seq::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
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
    return stream << value->AsString()->Get();
  } else if (value->IsSymbol()) {
    return stream << value->AsSymbol()->Get();
  } else if (value->IsNativeProcedure()) {
    const auto native = value->AsNativeProcedure();
    const auto& symbol = native->GetSymbol()->Get();
    return stream << "NativeProcedure(" << symbol << ")";
  } else if (value->IsClass()) {
    const auto cls = value->AsClass();
    ASSERT(cls);
    const auto& name = cls->GetName()->Get();
    return stream << "Class(" << name << ")";
  } else if (value->IsLambda()) {
    const auto lambda = value->AsLambda();
    stream << "Lambda(";
    if (lambda->HasSymbol())
      stream << lambda->GetSymbol()->Get();
    stream << ")";
    return stream;
  } else if (value->IsPair()) {
    const auto pair = value->AsPair();
    ASSERT(pair);
    stream << "(";
    if (pair->IsEmpty()) {
      stream << ")";
      return stream;
    }
    PrintValue(stream, pair->GetCar());
    auto next = pair->GetCdr();
    do {
      if (gel::IsNull(next)) {
        stream << ")";
        return stream;
      }
      if (!next->IsPair()) {
        stream << " ";
        PrintValue(stream, next);
        stream << ")";
        return stream;
      }
      stream << " ";
      PrintValue(stream, next->AsPair()->GetCar());
      next = next->AsPair()->GetCdr();
    } while (true);
  } else if (value->IsSet()) {
    stream << "(";
    auto remaining = value->AsSet()->GetSize();
    for (const auto& value : value->AsSet()->data()) {
      PrintValue(stream, value);
      if (--remaining > 0)
        stream << ", ";
    }
    stream << ")";
    return stream;
  }
  return stream << value->ToString();
}
}  // namespace gel