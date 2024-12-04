#include "gel/natives.h"

#include <fmt/args.h>
#include <fmt/base.h>
#include <fmt/format.h>

#include <exception>
#include <iostream>
#include <random>
#include <ranges>
#include <rpp/operators/fwd.hpp>
#include <rpp/operators/subscribe.hpp>

#include "gel/argument.h"
#include "gel/array.h"
#include "gel/collector.h"
#include "gel/common.h"
#include "gel/error.h"
#include "gel/gel.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/parser.h"
#include "gel/procedure.h"
#include "gel/runtime.h"
#include "gel/rx.h"
#include "gel/stack_frame.h"

namespace gel::proc {
NATIVE_PROCEDURE_F(gel_docs) {
  if (args.empty())
    return DoNothing();
  OptionalNativeArgument<0, Procedure> func(args);
  if (!func)
    return Throw(func.GetError());
  if (func->IsLambda()) {
    const auto lambda = func->AsLambda();
    std::stringstream ss;
    if (lambda->HasName())
      ss << lambda->GetName()->Get();
    ss << std::endl;
    ss << "([";
    const auto& args = lambda->GetArgs();
    if (!args.empty()) {
      auto remaining = args.size();
      for (const auto& arg : args) {
        ss << arg.GetName();
        if (--remaining > 0)
          ss << ", ";
      }
    }
    ss << "])";
    ss << std::endl;
    ss << "  ";
    if (lambda->HasDocstring())
      ss << lambda->GetDocstring()->Get();
    return ReturnNew<String>(ss.str());
  } else if (func->IsNativeProcedure()) {
    const auto native = func->AsNativeProcedure();
    std::stringstream ss;
    ss << native->GetSymbol()->Get() << std::endl;
    ss << "([";
    const auto& args = native->GetArgs();
    if (!args.empty()) {
      auto remaining = args.size();
      for (const auto& arg : args) {
        ss << arg.GetName();
        if (--remaining > 0)
          ss << ", ";
      }
    }
    ss << "])";
    ss << std::endl;
    ss << "  ";
    if (native->HasDocs())
      ss << native->GetDocs()->Get();
    return ReturnNew<String>(ss.str());
    return Return(func->AsNativeProcedure()->GetDocs());
  }
  return ThrowError(fmt::format("`{}` is not a Procedure", func->ToString()));
}

NATIVE_PROCEDURE_F(import) {
  ASSERT(!args.empty());
  const auto arg = args[0];
  ASSERT(arg);
  const auto symbol = ToSymbol(arg);
  if (!symbol) {
    LOG(FATAL) << arg << " is not a valid Symbol.";
    return false;
  }
  if (!GetRuntime()->Import(symbol, GetRuntime()->GetCurrentScope())) {
    LOG(FATAL) << "failed to import module: " << symbol;
    return false;
  }
  DLOG(INFO) << symbol << " imported!";
  return true;
}

NATIVE_PROCEDURE_F(print) {
  ASSERT(!args.empty());
  PrintValue(std::cout, args[0]) << std::endl;
  return DoNothing();
}

static std::random_device rand_device_{};   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static std::mt19937_64 mt(rand_device_());  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

NATIVE_PROCEDURE_F(random) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  return ReturnNew<Long>(mt());
}

NATIVE_PROCEDURE_F(rand_range) {
  ASSERT(HasRuntime());
  NativeArgument<0, Long> min(args);
  NativeArgument<1, Long> max(args);
  std::uniform_int_distribution<uint64_t> distribution(Long::Unbox(min), Long::Unbox(max));
  return ReturnNew<Long>(distribution(mt));
}

NATIVE_PROCEDURE_F(type) {
  ASSERT(!args.empty());
  NativeArgument<0> value(args);
  if (gel::IsNull(value))
    return ReturnNew<String>("Null");
  return Return(value->GetType()->GetName());
}

NATIVE_PROCEDURE_F(exit) {
  GetRuntime()->StopRunning();
  return true;
}

NATIVE_PROCEDURE_F(list) {
  if (args.empty())
    return Pair::Empty();
  Object* result = Pair::Empty();
  for (auto arg : std::ranges::reverse_view(args)) {
    result = Pair::New(arg, result);
  }
  return Return(result);  // TODO: use gel::ToList
}

NATIVE_PROCEDURE_F(format) {
  ASSERT(GetRuntime());
  ASSERT(args.size() >= 1);
  NativeArgument<0, String> format(args);
  const auto& fmt_val = format->Get();
  ASSERT(!fmt_val.empty());
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_args{};
  std::for_each(std::begin(args) + 1, std::end(args), [&fmt_args](Object* arg) {
    fmt_args.push_back(String::ValueOf(arg)->Get());
  });
  const auto result = fmt::vformat(fmt_val, fmt_args);
  ASSERT(!result.empty());
  return ReturnNew<String>(result);
}

// (set-car! <seq> <value>)
NATIVE_PROCEDURE_F(set_car) {
  NativeArgument<0, Pair> seq(args);
  NativeArgument<1> value(args);
  if (!value->IsDatum())
    return ThrowError(fmt::format("expected {} to be a Datum.", (*value)));
  SetCar(seq, value);
  return DoNothing();
}

NATIVE_PROCEDURE_F(set_cdr) {
  NativeArgument<0, Pair> seq(args);
  NativeArgument<1> value(args);
  if (!value->IsDatum())
    return ThrowError(fmt::format("expected {} to be a Datum.", (*value)));
  SetCdr(seq, value);
  return DoNothing();
}

NATIVE_PROCEDURE_F(array_new) {
  ASSERT(HasRuntime());
  if (args.empty())
    return ThrowError(fmt::format("expected args to not be empty"));
  const auto length = args.size();
  const auto result = Array<Object*>::New(length);
  ASSERT(result);
  for (auto idx = 0; idx < length; idx++) {
    ASSERT(args[idx]);
    result->Set(idx, args[idx]);
  }
  return Return(result);
}

NATIVE_PROCEDURE_F(array_get) {
  ASSERT(HasRuntime());
  if (args.size() != 2)
    return ThrowError(fmt::format("expected args to be: `<array> <index>`"));
  NativeArgument<0, ArrayBase> array(args);
  NativeArgument<1, Long> index(args);
  if (index->Get() > array->GetCapacity())
    return ThrowError(fmt::format("index `{}` is out of bounds for `{}`", index->Get(), (const gel::Object&)*array));
  const auto result = array->Get(index->Get());
  return Return(result ? result : Null());
}

NATIVE_PROCEDURE_F(array_set) {
  ASSERT(HasRuntime());
  if (args.size() != 3)
    return ThrowError(fmt::format("expected args to be: `<array> <index>`"));
  if (!gel::IsArray(args[0]))
    return ThrowError(fmt::format("expected `{}` to be an Array", (*args[0])));
  const auto array = (ArrayBase*)args[0];  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  ASSERT(array);
  if (!gel::IsLong(args[1]))
    return ThrowError(fmt::format("expected `{}` to be a Long.", (*args[1])));
  const auto index = Long::Unbox(args[1]);
  if (index > array->GetCapacity())
    return ThrowError(fmt::format("index `{}` is out of bounds for `{}`", index, (const gel::Object&)*array));
  array->Set(index, args[2]);
  return DoNothing();
}

NATIVE_PROCEDURE_F(array_length) {
  ASSERT(HasRuntime());
  NativeArgument<0, ArrayBase> array(args);
  return ReturnNew<Long>(array->GetCapacity());
}

#ifdef GEL_DEBUG

NATIVE_PROCEDURE_F(gel_minor_gc) {
  gel::MinorCollection();
  return DoNothing();
}

NATIVE_PROCEDURE_F(gel_major_gc) {
  gel::MajorCollection();
  return DoNothing();
}

NATIVE_PROCEDURE_F(gel_get_debug) {
#ifdef GEL_DEBUG
  return Return(Bool::True());
#else
  return Return(Bool::False());
#endif  // GEL_DEBUG
}

NATIVE_PROCEDURE_F(gel_get_frame) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  DLOG(INFO) << "stack frames:";
  StackFrameIterator iter(runtime->GetStackFrames());
  while (iter.HasNext()) {
    DLOG(INFO) << "- " << iter.Next();
  }
  return DoNothing();
}

NATIVE_PROCEDURE_F(gel_get_locals) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  LocalScope::RecursiveIterator iter(GetRuntime()->GetCurrentFrame()->GetLocals());
  return Return(gel::ToList<LocalScope::RecursiveIterator, LocalVariable*>(iter, [](LocalVariable* local) -> Object* {
    return String::New(local->GetName());
  }));
}

NATIVE_PROCEDURE_F(gel_get_classes) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  ClassListIterator iter;
  return Return(gel::ToList(iter));
}

NATIVE_PROCEDURE_F(gel_get_target_triple) {
  ASSERT(HasRuntime());
  return ReturnNew<String>(GEL_TARGET_TRIPLE);
}

NATIVE_PROCEDURE_F(gel_get_natives) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  const auto& natives = NativeProcedure::GetAll();
  Object* result = Null();
  for (const auto& native : natives) {
    result = Pair::New(String::ValueOf(native->GetSymbol()), result);
  }
  return Return(result);
}

NATIVE_PROCEDURE_F(gel_get_compile_time) {
  NativeArgument<0, Lambda> lambda(args);
  if (!lambda)
    return Throw(lambda.GetError());
  return ReturnNew<Long>(lambda->GetCompileTimeNanos());
}

#endif  // GEL_DEBUG

#ifdef GEL_ENABLE_RX

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
#endif  // GEL_ENABLE_RX

}  // namespace gel::proc