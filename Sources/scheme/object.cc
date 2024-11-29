#include "scheme/object.h"

#include <glog/logging.h>

#include <rpp/observers/fwd.hpp>
#include <rpp/observers/observer.hpp>
#include <sstream>
#include <string>
#include <utility>

#include "scheme/array.h"
#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/heap.h"
#include "scheme/macro.h"
#include "scheme/native_procedure.h"
#include "scheme/platform.h"
#include "scheme/pointer.h"
#include "scheme/procedure.h"
#include "scheme/runtime.h"
#include "scheme/rx.h"
#include "scheme/script.h"
#include "scheme/to_string_helper.h"

namespace scm {
#ifdef SCM_DISABLE_HEAP

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

#endif  // SCM_DISABLE_HEAP

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

ClassList Class::all_ = {};  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

auto Object::CreateClass() -> Class* {
  return Class::New(kClassName);
}

auto Object::raw_ptr() const -> Pointer* {
  const auto address = GetStartingAddress() - sizeof(Pointer);
  ASSERT(address >= UNALLOCATED);
  return Pointer::At(address);
}

void Object::Init() {
  InitClass();
  Class::InitClass();
  // exec
  Script::InitClass();
  Procedure::InitClass();
  Lambda::InitClass();
  NativeProcedure::InitClass();
  // types
  // numeric type(s)
  Number::InitClass();
  Long::InitClass();
  Double::InitClass();
  Pair::InitClass();
  Bool::Init();
  ArrayBase::InitClass();
  // string-like type(s)
  String::InitClass();
  Symbol::InitClass();
  Observable::InitClass();
  // error type(s)
  Error::InitClass();
}

auto Class::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Long::CreateClass() -> Class* {
  return Class::New(Number::GetClass(), kClassName);
}

auto Double::CreateClass() -> Class* {
  return Class::New(Number::GetClass(), kClassName);
}

auto Long::Unbox(Object* rhs) -> uint64_t {
  if (!rhs || !rhs->IsLong())
    throw Exception(fmt::format("expected `{}` to be a Long.", *rhs));
  return rhs->AsLong()->Get();
}

auto Class::ToString() const -> std::string {
  ToStringHelper<Class> helper;
  helper.AddField("name", GetName());
  helper.AddField("parent", GetParent());
  return helper;
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

auto Number::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Number::New(const uint64_t rhs) -> Number* {
  return Long::New(rhs);
}

auto Number::New(const double rhs) -> Number* {
  return Double::New(rhs);
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
  ToStringHelper<Long> helper;
  helper.AddField("value", Get());
  return helper;
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
  ToStringHelper<Double> helper;
  helper.AddField("value", Get());
  return helper;
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

auto Symbol::Equals(Object* rhs) const -> bool {
  if (!rhs->IsSymbol())
    return false;
  const auto other = rhs->AsSymbol();
  return Get() == other->Get();
}

auto StringObject::Equals(const std::string& rhs) const -> bool {
  return Get().compare(rhs) == 0;
}

auto Symbol::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
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

auto String::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
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
  ToStringHelper<String> helper;
  helper.AddField("value", Get());
  return helper;
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

auto Class::FindClass(String* name) -> Class* {
  for (const auto& cls : all_) {
    if (cls->GetName()->Equals(name))
      return cls;
  }
  return nullptr;
}

auto Class::FindClass(Symbol* name) -> Class* {
  return FindClass(scm::ToString(name));
}

auto Class::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Class::IsInstanceOf(Class* rhs) const -> bool {
  ASSERT(rhs);
  auto cls = this;
  while (cls) {
    if (cls->Equals(rhs))
      return true;
    cls = cls->GetParent();
  }
  return false;
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

auto Class::New(Class* parent, String* name) -> Class* {
  ASSERT(parent);
  ASSERT(name);
  const auto cls = new Class(parent, name);
  ASSERT(cls);
  all_.push_back(cls);
  return cls;
}

auto Class::New(Class* parent, const std::string& name) -> Class* {
  ASSERT(parent);
  ASSERT(!name.empty());
  return New(parent, String::New(name));
}

#ifdef SCM_ENABLE_RX

auto Observable::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Observable::ToString() const -> std::string {
  return ToStringHelper<Observable>{};
}

auto Observable::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Observable::ToObservable(Pair* list) -> rx::DynamicObjectObservable {
  ASSERT(list);
  return rx::source::create<Object*>([list](const auto& s) {
    Object* cell = list;
    while (!scm::IsNull(cell) && scm::IsPair(cell)) {
      const auto head = scm::Car(cell);
      ASSERT(head);
      s.on_next(head);
      cell = scm::Cdr(cell);
    }
    s.on_completed();
  });
}

auto Observable::Empty() -> Observable* {
  return new Observable(rx::empty());
}

auto Observable::New(Object* value) -> Observable* {
  if (scm::IsNull(value))
    return Empty();
  else if (scm::IsPair(value))
    return New(ToObservable(ToPair(value)));
  return New(rx::source::just(value));
}

static constexpr const auto kDoNothingOnError = [](std::exception_ptr) {
  return;  // do nothing
};

template <typename... Args>
static inline auto CallProcedure(Procedure* procedure) -> std::function<void(Args...)> {
  ASSERT(procedure);
  return [procedure](Args... args) {
    return GetRuntime()->Call(procedure, ObjectList{args...});
  };
}

auto Observer::CreateDynamicObserver(Procedure* on_next, Procedure* on_error, Procedure* on_completed)
    -> rx::DynamicObjectObserver {
  ASSERT(on_next);
  const auto on_next_callback = CallProcedure<scm::Object*>(on_next);
  const auto on_error_callback = kDoNothingOnError;  // TODO: fix
  const auto on_completed_callback = CallProcedure(on_completed);
  return rx::make_lambda_observer(CallProcedure<scm::Object*>(on_next), kDoNothingOnError, CallProcedure(on_completed));
}

auto Observer::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Observer::ToString() const -> std::string {
  return ToStringHelper<Observer>{};
}

auto Observer::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

#endif  // SCM_ENABLE_RX
}  // namespace scm