#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/observers/fwd.hpp>

#include "gel/common.h"
#include "gel/object.h"
#include "gel/rx.h"

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

auto Observable::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
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

auto Observer::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
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

auto PublishSubject::HashCode() const -> uword {
  return Subject::HashCode();
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

auto ReplaySubject::HashCode() const -> uword {
  return Subject::HashCode();
}

auto ReplaySubject::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto ReplaySubject::CreateClass() -> Class* {
  return Class::New(Subject::GetClass(), "ReplaySubject");
}

namespace proc {
#define NATIVE_RX_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(rx_##Name)

NATIVE_RX_PROCEDURE_F(observer) {
  NativeArgument<0, Procedure> on_next(args);
  OptionalNativeArgument<1, Procedure> on_error(args);
  OptionalNativeArgument<2, Procedure> on_completed(args);
  return ReturnNew<Observer>(on_next.GetValue(), on_error.GetValue(), on_completed.GetValue());
}

NATIVE_RX_PROCEDURE_F(first) {
  RequiredNativeArgument<0, Observable> source(args);
  source->Apply(rx::operators::first());
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(last) {
  RequiredNativeArgument<0, Observable> source(args);
  source->Apply(rx::operators::last());
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(skip) {
  RequiredNativeArgument<0, Observable> source(args);
  RequiredNativeArgument<1, Long> num_values(args);
  source->Apply(rx::operators::skip(num_values->Get()));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(take) {
  RequiredNativeArgument<0, Observable> source(args);
  RequiredNativeArgument<1, Long> num_values(args);
  source->Apply(rx::operators::take(num_values->Get()));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(filter) {
  RequiredNativeArgument<0, Observable> source(args);
  if (!source)
    return Throw(source.GetError());
  RequiredNativeArgument<1, Procedure> predicate(args);
  if (!predicate)
    return Throw(predicate.GetError());
  source->Apply(rx::operators::filter(rx::CallPredicate(GetRuntime(), predicate)));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(take_last) {
  RequiredNativeArgument<0, Observable> source(args);
  RequiredNativeArgument<1, Long> num_values(args);
  source->Apply(rx::operators::take_last(num_values->Get()));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(buffer) {
  RequiredNativeArgument<0, Observable> source(args);
  if (!source)
    return Throw(source.GetError());
  RequiredNativeArgument<1, Long> bucket_size(args);
  if (!bucket_size)
    return Throw(bucket_size.GetError());
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
  NativeArgument<0> source(args);
  if (!source)
    return Throw(source.GetError());
  NativeArgument<1> on_next_arg(args);
  if (!on_next_arg)
    return Throw(on_next_arg.GetError());
  if (on_next_arg->IsObserver()) {
    if (source.GetValue()->IsSubject()) {
      (source.GetValue())->AsSubject()->Subscribe(on_next_arg->AsObserver());
      return DoNothing();
    } else if (source.GetValue()->IsObservable()) {
      (source.GetValue()->AsObservable())->Subscribe(on_next_arg->AsObserver());
      return DoNothing();
    }
  }
  if (!on_next_arg->IsProcedure())
    return ThrowError(fmt::format("expected on_next arg `{}` to be a Procedure", (*on_next_arg)));
  OptionalNativeArgument<2, Procedure> on_error_arg(args);
  OptionalNativeArgument<3, Procedure> on_completed_arg(args);
  const auto on_next = rx::CallOnNext(runtime, on_next_arg->AsProcedure());
  const auto on_error = rx::CallOnError(runtime, on_error_arg);
  const auto on_completed = rx::CallOnComplete(runtime, on_completed_arg);
  if (source.GetValue()->IsSubject()) {
    (source.GetValue())->AsSubject()->Subscribe(on_next, on_error, on_completed);
    return DoNothing();
  } else if (source.GetValue()->IsObservable()) {
    (source.GetValue()->AsObservable())->GetValue().subscribe(on_next, on_error, on_completed);
    return DoNothing();
  }
  return ThrowError("not implemented");
}

#define CHECK_ARG_TYPE(Index, Name, Type)                  \
  const auto Name = args[Index];                           \
  if (!(Name) || !(Name->GetType()->IsInstanceOf((Type)))) \
    return ThrowError(fmt::format("expected arg #{} ({}) `{}` to be a `{}`", Index, #Name, (*Name), ((Type)->GetName())->Get()));

// (rx:map <func>)
NATIVE_RX_PROCEDURE_F(map) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (args.size() != 2)
    return ThrowError(fmt::format("expected args to be: `<observable> <func>`"));
  NativeArgument<0, Observable> source(args);
  if (!source)
    return Throw(source.GetError());
  NativeArgument<1, Procedure> func(args);
  if (!source)
    return Throw(source.GetError());
  source->AsObservable()->Apply(rx::map(runtime, func));
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(publish) {
  NativeArgument<0, Subject> subject(args);
  if (!subject)
    return Throw(subject.GetError());
  NativeArgument<1> value(args);
  subject->AsSubject()->Publish(value);
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(complete) {
  NativeArgument<0, Subject> subject(args);
  if (!subject)
    return Throw(subject.GetError());
  subject->AsSubject()->Complete();
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(publish_error) {
  NativeArgument<0, Subject> subject(args);
  if (!subject)
    return Throw(subject.GetError());
  NativeArgument<1, Error> value(args);
  if (!value)
    return Throw(value.GetError());
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
  CHECK_ARG_TYPE(1, predicate, Procedure::GetClass());
  source->AsObservable()->Apply(rx::operators::take_while([predicate, runtime](Object* value) {
    runtime->Call(predicate->AsProcedure(), {value});
    return gel::Truth(runtime->Pop());
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
    return String::New(local->GetName());
  }));
}
#endif  // GEL_DEBUG
}  // namespace proc
}  // namespace gel
#endif  // GEL_ENABLE_RX