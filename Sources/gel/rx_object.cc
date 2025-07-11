#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/observers/fwd.hpp>

#include "gel/common.h"
#include "gel/hashcode.h"
#include "gel/object.h"
#include "gel/rx.h"
#include "gel/subject.h"

#ifdef GEL_ENABLE_RX
#include "gel/error.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"

namespace gel {
auto Observable::CreateClass() -> Class* {
  return Class::New(Seq::GetClass(), kClassName);
}

auto Observable::ToString() const -> std::string {
  return ToStringHelper<Observable>{};
}

auto Observable::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Observable::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return kInvalidHashCode;
}

auto Observable::ToObservable(Pair* list) -> rx::DynamicObjectObservable {
  ASSERT(list);
  return rx::source::create<Object*>([list](const auto& s) {
    Object* cell = list;
    while (!cell->IsNil() && gel::IsPair(cell)) {
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
  if (value->IsNil())
    return Empty();
  else if (gel::IsPair(value))
    return New(ToObservable(ToPair(value)));
  else if (gel::IsSubject(value))
    return value->AsSubject()->ToObservable();
  return New(rx::source::just(value));
}

auto Observable::Compare(Object* rhs) const -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: not implemented
  return false;
}

auto Observable::New(const ObjectList& args) -> Observable* {
  if (args.empty() || args[0]->IsNil())
    return Empty();
  return New(args[0]);
}

auto Observer::CreateDynamicObserver(Fn* on_next_proc, Fn* on_error_proc, Fn* on_completed_proc)
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

auto Observer::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return kInvalidHashCode;
}

auto Observer::New() -> Observer* {
  return new Observer(
      rx::make_lambda_observer<gel::Object*>(rx::DoNothingOnNext(), rx::DoNothingOnError(), rx::DoNothingOnComplete()));
}

auto Observer::Compare(Object* rhs) const -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: not implemented
  return false;
}

auto Observer::New(const ObjectList& args) -> Observer* {
  if (args.empty())
    return New();
  const auto on_next = args[0];
  if (!on_next->IsFn())
    throw Exception(fmt::format("cannot create observer with on_next value of: {}", (*on_next)));
  const auto on_error = args.size() >= 2 ? args[1] : nullptr;
  if (on_error && !on_error->IsFn())
    throw Exception(fmt::format("cannot create observer with on_error value of: {}", (*on_error)));
  const auto on_complete = args.size() >= 3 ? args[2] : nullptr;
  if (on_complete && !on_complete->IsFn())
    throw Exception(fmt::format("cannot create observer with on_complete value of: {}", (*on_complete)));
  return New(on_next, on_error, on_complete);
}
namespace proc {
#define NATIVE_RX_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(rx_##Name)

NATIVE_RX_PROCEDURE_F(observer) {
  REQUIRED_NATIVE_ARG(0, Fn, on_next);
  OptionalNativeArgument<1, Fn> on_error(args);
  OptionalNativeArgument<2, Fn> on_completed(args);
  return ReturnNew<Observer>(on_next.GetValue(), on_error.GetValue(), on_completed.GetValue());
}

NATIVE_RX_PROCEDURE_F(first) {
  REQUIRED_NATIVE_ARG(0, Observable, source);
  source->Apply(rx::operators::first());
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(last) {
  REQUIRED_NATIVE_ARG(0, Observable, source);
  source->Apply(rx::operators::last());
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(skip) {
  REQUIRED_NATIVE_ARG(0, Observable, source);
  REQUIRED_NATIVE_ARG(1, Long, num_values);
  source->Apply(rx::operators::skip(num_values->Get()));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(take) {
  REQUIRED_NATIVE_ARG(0, Observable, source);
  REQUIRED_NATIVE_ARG(1, Long, num_values);
  source->Apply(rx::operators::take(num_values->Get()));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(filter) {
  REQUIRED_NATIVE_ARG(0, Observable, source);
  REQUIRED_NATIVE_ARG(1, Fn, filter);
  source->Apply(rx::operators::filter(rx::CallPredicate(GetRuntime(), filter)));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(take_last) {
  REQUIRED_NATIVE_ARG(0, Observable, source);
  REQUIRED_NATIVE_ARG(0, Long, num_values);
  source->Apply(rx::operators::take_last(num_values->Get()));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(buffer) {
  REQUIRED_NATIVE_ARG(0, Observable, source);
  REQUIRED_NATIVE_ARG(1, Long, bucket_size);
  const auto buffer = rx::operators::buffer(bucket_size->Get());
  const auto map = rx::operators::map([](ObjectList values) {
    return gel::ToList((const ObjectList&)values);
  });
  source->value_ = source->value_ | buffer | map;
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(observable) {
  // TODO: handle multiple args
  return ReturnNew<Observable>(args[0]);
}

NATIVE_RX_PROCEDURE_F(subscribe) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  REQUIRED_NATIVE_ARG(0, Object, source);
  REQUIRED_NATIVE_ARG(0, Object, on_next);
  if (on_next->IsObserver()) {
    if (source.GetValue()->IsSubject()) {
      (source.GetValue())->AsSubject()->Subscribe(on_next->AsObserver());
      return DoNothing();
    } else if (source.GetValue()->IsObservable()) {
      (source.GetValue()->AsObservable())->Subscribe(on_next->AsObserver());
      return DoNothing();
    }
  }
  ASSERT(on_next->IsFn());
  OptionalNativeArgument<2, Fn> on_error_arg(args);
  OptionalNativeArgument<3, Fn> on_completed_arg(args);
  const auto on_error = rx::CallOnError(runtime, on_error_arg);
  const auto on_completed = rx::CallOnComplete(runtime, on_completed_arg);
  if (source.GetValue()->IsSubject()) {
    (source.GetValue())->AsSubject()->Subscribe(rx::CallOnNext(runtime, on_next->AsFn()), on_error, on_completed);
    return DoNothing();
  } else if (source.GetValue()->IsObservable()) {
    (source.GetValue()->AsObservable())
        ->GetValue()
        .subscribe(rx::CallOnNext(runtime, on_next->AsFn()), on_error, on_completed);
    return DoNothing();
  }
  return ThrowError("not implemented");
}

#define CHECK_ARG_TYPE(Index, Name, Type)                  \
  const auto Name = args[Index];                           \
  if (!(Name) || !(Name->GetType()->IsInstanceOf((Type)))) \
    return ThrowError(                                     \
        fmt::format("expected arg #{} ({}) `{}` to be a `{}`", Index, #Name, (*Name), ((Type)->GetName())->Get()));

// (rx:map <func>)
NATIVE_RX_PROCEDURE_F(map) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (args.size() != 2)
    return ThrowError(fmt::format("expected args to be: `<observable> <func>`"));
  REQUIRED_NATIVE_ARG(0, Observable, source);
  REQUIRED_NATIVE_ARG(1, Fn, callback);
  source->AsObservable()->Apply(rx::map(runtime, callback));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(publish) {
  REQUIRED_NATIVE_ARG(0, Subject, subject);
  REQUIRED_NATIVE_ARG(1, Object, value);
  subject->AsSubject()->Publish(value);
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(complete) {
  REQUIRED_NATIVE_ARG(0, Subject, subject);
  subject->AsSubject()->Complete();
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(publish_error) {
  REQUIRED_NATIVE_ARG(0, Subject, subject);
  REQUIRED_NATIVE_ARG(1, Error, value);
  try {
    throw Exception(value->GetMessage()->Get());
  } catch (const Exception& exc) {
    subject->AsSubject()->OnError(std::current_exception());
  }
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(take_while) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (args.size() != 2)
    return ThrowError(fmt::format("expected args to be: `<observable> <func>`"));
  CHECK_ARG_TYPE(0, source, Observable::GetClass());
  CHECK_ARG_TYPE(1, predicate, Fn::GetClass());
  source->AsObservable()->Apply(rx::operators::take_while([predicate, runtime](Object* value) {
    return gel::Truth(runtime->CallPop(*(predicate->AsFn()), {value}));
  }));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(replay_subject) {
  if (!args.empty())
    return ThrowError(fmt::format("expected args to be empty."));
  return ReturnNew<ReplaySubject>();
}

NATIVE_RX_PROCEDURE_F(publish_subject) {
  if (!args.empty())
    return ThrowError(fmt::format("expected args to be empty."));
  return ReturnNew<PublishSubject>();
}

#ifdef GEL_DEBUG
NATIVE_RX_PROCEDURE_F(get_operators) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  LocalScope::RecursiveIterator iter(rx::GetRxScope());
  return Return(gel::ToList<LocalScope::RecursiveIterator, LocalVariable*>(iter, [](LocalVariable* local) -> Object* {
    return String::New(local->GetSymbol());
  }));
}
#endif  // GEL_DEBUG
}  // namespace proc
}  // namespace gel
#endif  // GEL_ENABLE_RX