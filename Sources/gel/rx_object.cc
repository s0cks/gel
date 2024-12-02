#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/observers/fwd.hpp>

#include "gel/object.h"
#include "gel/rx.h"

#ifdef GEL_ENABLE_RX
#include "gel/error.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"

namespace gel {
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
    while (!gel::IsNull(cell) && gel::IsPair(cell)) {
      const auto head = gel::Car(cell);
      ASSERT(head);
      s.on_next(head);
      cell = gel::Cdr(cell);
    }
    s.on_completed();
  });
}

auto Observable::Empty() -> Observable* {
  return new Observable(rx::empty());
}

auto Observable::New(Object* value) -> Observable* {
  if (gel::IsNull(value))
    return Empty();
  else if (gel::IsPair(value))
    return New(ToObservable(ToPair(value)));
  else if (gel::IsSubject(value))
    return value->AsSubject()->ToObservable();
  return New(rx::source::just(value));
}

auto Observable::New(const ObjectList& args) -> Observable* {
  if (args.empty() || gel::IsNull(args[0]))
    return Empty();
  return New(args[0]);
}

auto Observer::CreateDynamicObserver(Procedure* on_next_proc, Procedure* on_error_proc, Procedure* on_completed_proc)
    -> rx::DynamicObjectObserver {
  ASSERT(on_next_proc);
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto on_next = rx::CallOnNext(runtime, on_next_proc);
  const auto on_error = rx::CallOnError(runtime, on_error_proc);
  const auto on_completed = rx::CallOnComplete(runtime, on_completed_proc);
  return rx::make_lambda_observer<gel::Object*>(on_next, on_error, on_completed);
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

auto Observer::New() -> Observer* {
  return new Observer(
      rx::make_lambda_observer<gel::Object*>(rx::DoNothingOnNext(), rx::DoNothingOnError(), rx::DoNothingOnComplete()));
}

auto Observer::New(const ObjectList& args) -> Observer* {
  if (args.empty())
    return New();
  const auto on_next = args[0];
  if (!on_next->IsProcedure())
    throw Exception(fmt::format("cannot create observer with on_next value of: {}", (*on_next)));
  const auto on_error = args.size() >= 2 ? args[1] : nullptr;
  if (on_error && !on_error->IsProcedure())
    throw Exception(fmt::format("cannot create observer with on_error value of: {}", (*on_error)));
  const auto on_complete = args.size() >= 3 ? args[2] : nullptr;
  if (on_complete && !on_complete->IsProcedure())
    throw Exception(fmt::format("cannot create observer with on_complete value of: {}", (*on_complete)));
  return New(on_next, on_error, on_complete);
}

auto Subject::to_exception_ptr(Error* error) -> std::exception_ptr {
  ASSERT(error && !error->GetMessage()->Get().empty());
  return std::make_exception_ptr(Exception(String::Unbox(error->GetMessage())));
}

auto Subject::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), kClassName);
}

auto Subject::ToString() const -> std::string {
  return ToStringHelper<Subject>{};
}

auto Subject::New(const ObjectList& args) -> Subject* {
  NOT_IMPLEMENTED(FATAL);
}

auto PublishSubject::New(const ObjectList& args) -> PublishSubject* {
  ASSERT(args.empty());
  return New();
}

auto PublishSubject::ToString() const -> std::string {
  return ToStringHelper<PublishSubject>{};
}

auto PublishSubject::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto PublishSubject::CreateClass() -> Class* {
  return Class::New(Subject::GetClass(), "PublishSubject");
}

auto ReplaySubject::New(const ObjectList& args) -> ReplaySubject* {
  ASSERT(args.empty());
  return New();
}

auto ReplaySubject::ToString() const -> std::string {
  return ToStringHelper<ReplaySubject>{};
}

auto ReplaySubject::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto ReplaySubject::CreateClass() -> Class* {
  return Class::New(Subject::GetClass(), "ReplaySubject");
}

}  // namespace gel
#endif  // GEL_ENABLE_RX