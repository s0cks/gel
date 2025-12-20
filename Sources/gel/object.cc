#include "object.h"

#include <exception>
#include <glog/logging.h>
#include <iterator>
#include <rpp/observers/fwd.hpp>
#include <rpp/observers/observer.hpp>
#include <rpp/sources/fwd.hpp>
#include <sstream>
#include <string>
#include <utility>

#include "array.h"
#include "binary_op.h"
#include "boolean.h"
#include "buffer.h"
#include "common.h"
#include "event_emitter.h"
#include "event_loop.h"
#include "exception.h"
#include "expr/expression.h"
#include "hashcode.h"
#include "heap.h"
#include "namespace.h"
#include "natives.h"
#include "number.h"
#include "pair.h"
#include "platform.h"
#include "pointer.h"
#include "runtime.h"
#include "rx.h"
#include "subject.h"
#include "symbol.h"
#include "to_string_helper.h"
#include "type.h"
#include "types.h"

namespace gel {
DEFINE_NEW_OPERATOR(Seq);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Field);         // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Bool);          // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Number);        // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Double);        // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Long);          // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(String);        // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Symbol);        // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Macro);         // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Fn);            // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Lambda);        // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(NativeFn);      // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Pair);          // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Script);        // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Error);         // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Namespace);     // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Set);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Iterator);      // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Map);           // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Module);        // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(EventLoop);     // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Timer);         // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(EventEmitter);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Observer);      // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(Observable);    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

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
    return Nil::Get();                              \
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
  Fn::InitClass();
  Lambda::InitClass();
  NativeFn::Init();
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

#ifdef GEL_ENABLE_RX
  Observable::InitClass();
  Observer::InitClass();
  Subject::InitClass();
  ReplaySubject::InitClass();
  PublishSubject::InitClass();
#endif  // GEL_ENABLE_RX
}

auto Object::FieldAddr(Field* field) const -> Object** {
  ASSERT(field && field->GetOffset() > 0);
  return FieldAddrAtOffset(field->GetOffset());
}
auto Seq::New(const ObjectList& args) -> Seq* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return nullptr;
}

auto Seq::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Seq");
}

auto Seq::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return kInvalidHashCode;
}

auto Seq::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

template <>
auto PrintValue(std::ostream& stream, Nil& value) -> std::ostream& {
  return stream << "nil";
}

template <>
auto PrintValue(std::ostream& stream, Bool& value) -> std::ostream& {
  return stream << (value.Get() ? "true" : "false");
}

template <>
auto PrintValue(std::ostream& stream, Double& value) -> std::ostream& {
  return stream << value.Get();
}

template <>
auto PrintValue(std::ostream& stream, Long& value) -> std::ostream& {
  return stream << value.Get();
}

template <>
auto PrintValue(std::ostream& stream, String& value) -> std::ostream& {
  return stream << value.Get();
}

template <>
auto PrintValue(std::ostream& stream, Symbol& value) -> std::ostream& {
  return stream << value.GetFullyQualifiedName();
}

template <>
auto PrintValue(std::ostream& stream, NativeFn& value) -> std::ostream& {
  const auto& symbol = value.GetSymbol()->GetFullyQualifiedName();
  return stream << "NativeFn(" << symbol << ")";
}

template <>
auto PrintValue(std::ostream& stream, Class& value) -> std::ostream& {
  return stream << "Class(" << value.GetName()->Get() << ")";
}

template <>
auto PrintValue(std::ostream& stream, Pair& value) -> std::ostream& {
  stream << "(";
  if (value.IsEmpty()) {
    stream << ")";
    return stream;
  }
  PrintValue(stream, value.GetFirst());
  auto next = value.GetSecond();
  do {
    if (next->IsNil()) {
      stream << ")";
      return stream;
    }
    if (!next->IsPair()) {
      stream << " . ";
      PrintValue(stream, next);
      stream << ")";
      return stream;
    }
    stream << " ";
    PrintValue(stream, next->AsPair()->GetFirst());
    next = next->AsPair()->GetSecond();
  } while (true);
}

template <>
auto PrintValue(std::ostream& stream, Lambda& value) -> std::ostream& {
  stream << "Lambda(";
  if (value.HasSymbol())
    stream << value.GetSymbol()->GetFullyQualifiedName();
  stream << ")";
  return stream;
}

template <>
auto PrintValue(std::ostream& stream, Set& value) -> std::ostream& {
  stream << "#{";
  auto remaining = value.GetSize();
  for (const auto& value : value) {
    PrintValue(stream, value);
    if (--remaining > 0)
      stream << " ";
  }
  stream << "}";
  return stream;
}

template <>
auto PrintValue(std::ostream& stream, Map& value) -> std::ostream& {
  stream << "{";
  auto remaining = value.GetSize();
  for (const auto& [first, second] : value) {
    PrintValue(stream, (Object*)first);
    stream << ": ";
    PrintValue(stream, second);
    if (--remaining > 0)
      stream << ", ";
  }
  stream << "}";
  return stream;
}

auto PrintValue(std::ostream& stream, Object* value) -> std::ostream& {
  ASSERT(value);
  if (value->IsBool()) {
    return PrintValue(stream, *(value->AsBool()));
  } else if (value->IsDouble()) {
    return PrintValue(stream, *(value->AsDouble()));
  } else if (value->IsLong()) {
    return PrintValue(stream, *(value->AsLong()));
  } else if (value->IsString()) {
    return PrintValue(stream, *(value->AsString()));
  } else if (value->IsSymbol()) {
    return PrintValue(stream, *(value->AsSymbol()));
  } else if (value->IsNativeFn()) {
    return PrintValue(stream, *(value->AsNativeFn()));
  } else if (value->IsClass()) {
    return PrintValue(stream, *(value->AsClass()));
  } else if (value->IsLambda()) {
    return PrintValue(stream, *(value->AsLambda()));
  } else if (value->IsPair()) {
    return PrintValue(stream, *(value->AsPair()));
  } else if (value->IsSet()) {
    return PrintValue(stream, *(value->AsSet()));
  } else if (value->IsMap()) {
    return PrintValue(stream, *(value->AsMap()));
  } else if (value->IsNil())
    return PrintValue(stream, *(value->AsNil()));
  return stream << value->ToString();
}
}  // namespace gel
