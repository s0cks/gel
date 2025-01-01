#include "gel/object.h"

#include <glog/logging.h>

#include <exception>
#include <iterator>
#include <rpp/observers/fwd.hpp>
#include <rpp/observers/observer.hpp>
#include <rpp/sources/fwd.hpp>
#include <sstream>
#include <string>
#include <utility>

#include "gel/array.h"
#include "gel/buffer.h"
#include "gel/common.h"
#include "gel/event_loop.h"
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
#include "gel/symbol.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"
namespace gel {
#ifdef GEL_DISABLE_HEAP

#define DEFINE_NEW_OPERATOR(Name)                     \
  auto Name::operator new(const size_t sz) -> void* { \
    return malloc(sz);                                \
  }

#else

#define DEFINE_NEW_OPERATOR(Name)                                             \
  auto Name::operator new(const size_t sz) -> void* {                         \
    const auto alloc_size = Name::kClass ? kClass->GetAllocationSize() : sz;  \
    const auto heap = Heap::GetHeap();                                        \
    ASSERT(heap);                                                             \
    const auto address = heap->TryAllocate(alloc_size > 0 ? alloc_size : sz); \
    ASSERT(address != UNALLOCATED);                                           \
    return reinterpret_cast<void*>(address);                                  \
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
  return Class::New(Class::kObjectClassId, kClassName);
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
  Class::Init();
  Field::InitClass();
  String::InitClass();
  Symbol::InitClass();
  Namespace::InitClass();
  Module::InitClass();
  Seq::InitClass();
  Map::InitClass();
  Procedure::InitClass();
  Lambda::InitClass();
  NativeProcedure::Init();
  Buffer::Init();
  Script::InitClass();
  Number::InitClass();
  Long::InitClass();
  Double::InitClass();
  Pair::InitClass();
  Bool::Init();
  ArrayBase::InitClass();
  Macro::InitClass();
  Error::InitClass();
  Set::InitClass();
  Expression::Init();
  EventLoop::Init();
#ifdef GEL_ENABLE_RX
  Observable::InitClass();
  Observer::InitClass();
  Subject::InitClass();
  ReplaySubject::InitClass();
  PublishSubject::InitClass();
#endif  // GEL_ENABLE_RX
}

auto Long::CreateClass() -> Class* {
  return Class::New(Class::kLongClassId, Number::GetClass(), kClassName);
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
  return Class::New(kClassId, Number::GetClass(), kClassName);
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
  return Class::New(kClassId, Object::GetClass(), kClassName);
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

auto Object::FieldAddr(Field* field) const -> Object** {
  ASSERT(field && field->GetOffset() > 0);
  return FieldAddrAtOffset(field->GetOffset());
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
  return Class::New(Seq::GetClass(), kClassName);
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
  if (HasCar())
    CombineHash(hash, GetCar());
  if (HasCdr())
    CombineHash(hash, GetCdr());
  return hash;
}

auto StringObject::Equals(Object* rhs) const -> bool {
  if (!rhs || !(rhs->IsString() || rhs->IsSymbol()))
    return false;
  if (rhs->IsSymbol())
    return Equals(rhs->AsSymbol()->GetFullyQualifiedName());
  ASSERT(rhs->IsString());
  return Equals(rhs->AsString()->Get());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

auto StringObject::Equals(const std::string& rhs) const -> bool {
  return Get().compare(rhs) == 0;
}

auto String::New() -> String* {
  static auto kEmptyString = new String();
  ASSERT(kEmptyString);
  return kEmptyString;
}

auto String::New(Symbol* rhs) -> String* {
  ASSERT(rhs);
  return New(rhs->GetFullyQualifiedName());
}

auto String::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto String::Equals(const std::string& rhs) const -> bool {
  return StringObject::Equals(rhs);
}

auto String::Equals(Object* rhs) const -> bool {
  return StringObject::Equals(rhs);
}

auto String::New(const ObjectList& args) -> String* {
  if (args.empty() || gel::IsNull(args[0]))
    return New();
  if (args[0]->IsString())
    return String::New(args[0]->AsString()->Get());
  else if (gel::IsBuffer(args[0])) {
    const auto buffer = args[0]->AsBuffer();
    ASSERT(buffer);
    std::string value((const char*)buffer->data(), buffer->GetLength());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    return String::New(value);
  }
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
    return String::New(rhs->AsSymbol()->GetFullyQualifiedName());
  std::stringstream ss;
  if (rhs->IsBool()) {
    ss << (Bool::Unbox(rhs->AsBool()) ? "#t" : "#f");
  } else if (rhs->IsLong()) {
    ss << rhs->AsLong()->Get();
  } else if (rhs->IsDouble()) {
    ss << rhs->AsDouble()->Get();
  } else if (rhs->IsSymbol()) {
    ss << rhs->AsSymbol()->GetFullyQualifiedName();
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

auto Set::Of(Object* value) -> Set* {
  if (gel::IsNull(value))
    return Set::Of();
  else if (value->IsSet())
    return value->AsSet();
  else if (value->IsPair()) {
    if (value->AsPair()->IsEmpty())
      return Of();
    else if (value->AsPair()->IsTuple())
      return Of({
          value->AsPair()->GetCar(),
          value->AsPair()->GetCdr(),
      });
    ObjectList values;
    auto v = value;
    while (!gel::IsNull(v)) {
      values.push_back(gel::Car(v));
      v = gel::Cdr(v);
    }
    return Of(StorageType(std::begin(values), std::end(values)));
  }
  return Of(StorageType{value});
}

auto Set::New(const ObjectList& args) -> Set* {
  if (args.empty())
    return Of();
  else if (args.size() == 1)
    return Of(args[0]);
  StorageType data(args.begin(), args.end());
  return Of(data);
}

auto Set::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Seq::GetClass(), "Set");
}

auto Map::Get(Object* key) const -> Object* {
  ASSERT(key);
  const auto pos = Find(key);
  if (pos == std::end(data()))
    return Null();
  return pos->second;
}

auto Map::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

auto Map::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsMap())
    return false;
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Map::ToString() const -> std::string {
  ToStringHelper<Map> helper;
  helper.AddField("size", GetSize());
  return helper;
}

auto Map::New(const ObjectList& args) -> Map* {
  ASSERT(args.empty() || (args.size() % 2 == 0));
  if (args.empty())
    return Map::New();
  StorageType data{};
  for (auto idx = 0; idx < args.size(); idx += 2) {
    const auto key = args[idx];
    ASSERT(key);
    const auto value = args[idx + 1];
    ASSERT(value);
    data.insert({key, value});  // TODO: prolly should check this insertion
  }
  return Map::New(data);
}

auto Map::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Seq::GetClass(), "Map");
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
    return stream << value->AsSymbol()->GetFullyQualifiedName();
  } else if (value->IsNativeProcedure()) {
    const auto native = value->AsNativeProcedure();
    const auto& symbol = native->GetSymbol()->GetFullyQualifiedName();
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
      stream << lambda->GetSymbol()->GetFullyQualifiedName();
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

namespace proc {
#define SET_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(set_##Name)

SET_PROCEDURE_F(contains) {
  NativeArgument<0, Set> set(args);
  if (!set)
    return Throw(set);
  NativeArgument<1> value(args);
  if (!value)
    return Throw(value);
  return ReturnBool(set->Contains(value));
}

SET_PROCEDURE_F(count) {
  NativeArgument<0, Set> set(args);
  if (!set)
    return Throw(set);
  return ReturnLong(set->GetSize());
}

SET_PROCEDURE_F(empty) {
  NativeArgument<0, Set> set(args);
  if (!set)
    return Throw(set);
  return ReturnBool(set->IsEmpty());
}

#undef SET_PROCEDURE_F

#define MAP_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(map_##Name)
MAP_PROCEDURE_F(contains) {
  NativeArgument<0, Map> m(args);
  if (!m)
    return Throw(m);
  NativeArgument<1> key(args);
  if (!key)
    return Throw(key);
  return ReturnBool(m->Contains(key));
}

MAP_PROCEDURE_F(get) {
  NativeArgument<0, Map> m(args);
  if (!m)
    return Throw(m);
  NativeArgument<1> key(args);
  if (!key)
    return Throw(key);
  return Return(m->Get(key));
}

MAP_PROCEDURE_F(size) {
  NativeArgument<0, Map> m(args);
  if (!m)
    return Throw(m);
  return ReturnLong(m->GetSize());
}

MAP_PROCEDURE_F(empty) {
  NativeArgument<0, Map> m(args);
  if (!m)
    return Throw(m);
  return ReturnBool(m->IsEmpty());
}
#undef MAP_PROCEDURE_F
}  // namespace proc
}  // namespace gel