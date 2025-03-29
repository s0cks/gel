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
#include "gel/binary_op.h"
#include "gel/buffer.h"
#include "gel/common.h"
#include "gel/event_emitter.h"
#include "gel/event_loop.h"
#include "gel/exception.h"
#include "gel/expression.h"
#include "gel/heap.h"
#include "gel/namespace.h"
#include "gel/natives.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/runtime.h"
#include "gel/rx.h"
#include "gel/symbol.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"
#include "gel/types.h"

namespace gel {
DEFINE_NEW_OPERATOR(Seq);              // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Field);            // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Bool);             // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Number);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Double);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Long);             // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(String);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Symbol);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Macro);            // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Procedure);        // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Lambda);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Constructor);      // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(NativeProcedure);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Pair);             // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Script);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Error);            // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Namespace);        // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Set);              // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Iterator);         // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Map);              // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Module);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(EventLoop);        // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Timer);            // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Buffer);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(EventEmitter);     // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Observer);         // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Observable);       // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Subject);          // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(PublishSubject);   // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(ReplaySubject);    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

#undef DEFINE_NEW_OPERATOR

#define DEFINE_INIT_CLASS(Name)  \
  Class* Name::kClass = nullptr; \
  void Name::InitClass() {       \
    ASSERT(kClass == nullptr);   \
    kClass = CreateClass();      \
    ASSERT(kClass);              \
  }
DEFINE_INIT_CLASS(Object);
FOR_EACH_TYPE(DEFINE_INIT_CLASS);
#undef DEFINE_TYPE_INIT

auto Object::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  const auto cls = Class::New(Class::kObjectClassId, kClassName);
  ASSERT(cls);
  using namespace proc;
  cls->AddFunction(object_hashcode::Get()->GetNative());
  return cls;
}

#define DECLARE_OBJECT_BINARY_OP(Name)              \
  auto Object::Name(Object* rhs) const -> Object* { \
    NOT_IMPLEMENTED(ERROR);                         \
    DLOG(ERROR) << ToString();                      \
    return Null();                                  \
  }

DECLARE_OBJECT_BINARY_OP(Add);
DECLARE_OBJECT_BINARY_OP(Subtract);
DECLARE_OBJECT_BINARY_OP(Multiply);
DECLARE_OBJECT_BINARY_OP(Divide);
DECLARE_OBJECT_BINARY_OP(Modulus);
DECLARE_OBJECT_BINARY_OP(Eq);
DECLARE_OBJECT_BINARY_OP(BitAnd);
DECLARE_OBJECT_BINARY_OP(BitOr);
DECLARE_OBJECT_BINARY_OP(BitXor);
DECLARE_OBJECT_BINARY_OP(ShiftLeft);
DECLARE_OBJECT_BINARY_OP(ShiftRight);
DECLARE_OBJECT_BINARY_OP(GreaterThan);
DECLARE_OBJECT_BINARY_OP(GreaterThanEqual);
DECLARE_OBJECT_BINARY_OP(LessThan);
DECLARE_OBJECT_BINARY_OP(LessThanEqual);
DECLARE_OBJECT_BINARY_OP(InstanceOf);

auto Object::Cons(Object* rhs) const -> Object* {
  return gel::Cons(const_cast<Object*>(this), rhs);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

#undef DECLARE_OBJECT_BINARY_OP

auto Object::Compare(Object* rhs) const -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Object::VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!VisitPointerPointer(vis, &kClass))
    return false;
  return true;
}

#define DEFINE_VISIT_CLASS_POINTER_POINTER(Name)                            \
  auto Name::VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool { \
    ASSERT(vis);                                                            \
    if (!VisitPointerPointer(vis, &kClass))                                 \
      return false;                                                         \
    return true;                                                            \
  }
FOR_EACH_TYPE(DEFINE_VISIT_CLASS_POINTER_POINTER)
#undef DEFINE_VISIT_CLASS_POINTER_POINTER

void Object::Init() {
  using namespace proc;
  InitNative<object_hashcode>();
  Class::Init();
  InitClass();
  Class::InitClass();
  Field::InitClass();
  String::InitClass();
  Symbol::Init();
  Argument::InitClass();
  Namespace::Init();
  Module::Init();
  Seq::InitClass();
  Map::Init();
  Procedure::InitClass();
  Constructor::InitClass();
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
  Macro::Init();
  Error::InitClass();
  Set::Init();
  Expression::Init();
  EventLoop::Init();
  EventEmitter::Init();
  Iterator::Init();

#ifdef GEL_ENABLE_GLM
  Vec2::InitClass();
  Vec3::InitClass();
#endif  // GEL_ENABLE_GLM

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
  else if (args.size() == 1) {
    if (args[0]->IsNumber() && (args[0]->AsNumber()->GetLong() == 0))
      return False();
    return Box(gel::Truth(args[0]));
  }
  return Box(gel::Truth(gel::ToList(args)));
}

auto Bool::ToString() const -> std::string {
  return Get() ? "#T" : "#F";
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

auto Bool::Compare(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsBool())
    return false;
  return Get() < rhs->AsBool()->Get();
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
  if (gel::IsNull(rhs))
    throw IllegalArgumentException("rhs equals '()");
  else if (!rhs->IsNumber())
    throw IllegalArgumentException(fmt::format("expected `{}` to be a Number.", (*rhs)));
  return Long::New(GetLong() & rhs->AsNumber()->GetLong());
}

auto Long::BitOr(Object* rhs) const -> Object* {
  if (gel::IsNull(rhs))
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

auto Pair::New(const ObjectList& args) -> Pair* {
  if (args.empty())
    return Pair::Empty();
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Pair::Compare(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsPair())
    return false;
  if (GetFirst() < rhs->AsPair()->GetFirst())
    return true;
  return GetSecond() < rhs->AsPair()->GetSecond();
}

Field* Pair::kFirstField = nullptr;
Field* Pair::kSecondField = nullptr;
auto Pair::CreateClass() -> Class* {
  const auto cls = Class::New(Seq::GetClass(), kClassName);
  ASSERT(cls);
  kFirstField = cls->AddField("first");
  ASSERT(kFirstField);
  kSecondField = cls->AddField("second");
  ASSERT(kSecondField);
  return cls;
}

auto Pair::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (HasFirst()) {
    if (!vis->Visit(GetFirst()))
      return false;
  }
  if (HasSecond()) {
    if (!vis->Visit(GetSecond()))
      return false;
  }
  return true;
}

auto Pair::Equals(Object* rhs) const -> bool {
  if (!rhs->IsPair())
    return false;
  const auto other = rhs->AsPair();
  return GetFirst()->Equals(other->GetFirst()) && GetSecond()->Equals(other->GetSecond());
}

auto Pair::ToString() const -> std::string {
  ToStringHelper<Pair> helper;
  helper.AddField("first", GetFirst());
  helper.AddField("second", GetSecond());
  return helper;
}

static Pair* kEmptyPair = nullptr;
auto Pair::Empty() -> Pair* {
  if (kEmptyPair)
    return kEmptyPair;
  return kEmptyPair = Pair::NewEmpty();
}

auto Pair::VisitEmptyPointerPointer(const std::function<bool(Pointer**)>& vis) -> bool {
  ASSERT(vis);
  ASSERT(kEmptyPair);
  auto empty = kEmptyPair->raw_ptr();
  if (!vis(&empty))
    return false;
  if (!kEmptyPair->raw_ptr()->Equals(empty))
    kEmptyPair = empty->As<Pair>();
  return true;
}

auto Pair::VisitEmptyPointerPointer(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  ASSERT(kEmptyPair);
  auto empty = kEmptyPair->raw_ptr();
  if (!vis->Visit(&empty))
    return false;
  if (!kEmptyPair->raw_ptr()->Equals(empty))
    kEmptyPair = empty->As<Pair>();
  return true;
}

auto Pair::HashCode() const -> uword {
  uword hash = 0;
  if (HasFirst())
    CombineHash(hash, GetFirst());
  if (HasSecond())
    CombineHash(hash, GetSecond());
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

auto String::Compare(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsString())
    return false;
  return Get().compare(rhs->AsString()->Get());
}

auto String::ToBuffer() const -> Buffer* {
  return Buffer::Copy(Get());
}

auto String::Equals(const std::string& rhs) const -> bool {
  return StringObject::Equals(rhs);
}

auto String::Equals(Object* rhs) const -> bool {
  return StringObject::Equals(rhs);
}

auto String::Eq(Object* rhs) const -> Object* {
  return Bool::Box(Equals(rhs));
}

auto String::New(const ObjectList& args) -> String* {
  if (args.empty() || gel::IsNull(args[0]))
    return New();
  if (args[0]->IsString())
    return String::New(args[0]->AsString()->Get());
  else if (gel::IsBuffer(args[0])) {
    const auto buffer = args[0]->AsBuffer();
    ASSERT(buffer);
    std::string value((const char*)buffer->data(), buffer->GetCapacity());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
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
      PrintValue(ss, pair->GetFirst());
      auto next = pair->GetSecond();
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
        PrintValue(ss, next->AsPair()->GetFirst());
        next = next->AsPair()->GetSecond();
      } while (true);
    }
  } else if (rhs->IsError()) {
    ss << rhs->AsError()->GetMessage()->Get();
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
    PrintValue(stream, pair->GetFirst());
    auto next = pair->GetSecond();
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
      PrintValue(stream, next->AsPair()->GetFirst());
      next = next->AsPair()->GetSecond();
    } while (true);
  } else if (value->IsSet()) {
    stream << "#{";
    auto remaining = value->AsSet()->GetSize();
    for (const auto& value : value->AsSet()->data()) {
      PrintValue(stream, value);
      if (--remaining > 0)
        stream << " ";
    }
    stream << "}";
    return stream;
  } else if (value->IsVec2()) {
    stream << "[";
    stream << value->AsVec2()->GetX() << " ";
    stream << value->AsVec2()->GetY();
    stream << "]";
    return stream;
  } else if (value->IsVec3()) {
    stream << "[";
    stream << value->AsVec3()->GetX() << " ";
    stream << value->AsVec3()->GetY() << " ";
    stream << value->AsVec3()->GetZ();
    stream << "]";
    return stream;
  }
  return stream << value->ToString();
}
}  // namespace gel