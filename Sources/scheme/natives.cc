#include "scheme/natives.h"

#include <fmt/args.h>
#include <fmt/base.h>
#include <fmt/format.h>

#include <exception>
#include <iostream>
#include <random>
#include <ranges>
#include <rpp/operators/fwd.hpp>
#include <rpp/operators/subscribe.hpp>

#include "scheme/argument.h"
#include "scheme/array.h"
#include "scheme/collector.h"
#include "scheme/common.h"
#include "scheme/error.h"
#include "scheme/local.h"
#include "scheme/local_scope.h"
#include "scheme/native_procedure.h"
#include "scheme/object.h"
#include "scheme/parser.h"
#include "scheme/procedure.h"
#include "scheme/runtime.h"
#include "scheme/rx.h"
#include "scheme/scheme.h"
#include "scheme/stack_frame.h"

namespace scm::proc {
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
  if (scm::IsNull(value))
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
  return Return(result);  // TODO: use scm::ToList
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
    return ThrowError(fmt::format("index `{}` is out of bounds for `{}`", index->Get(), (const scm::Object&)*array));
  return Return(array->Get(index->Get()));
}

NATIVE_PROCEDURE_F(array_set) {
  ASSERT(HasRuntime());
  if (args.size() != 3)
    return ThrowError(fmt::format("expected args to be: `<array> <index>`"));
  if (!scm::IsArray(args[0]))
    return ThrowError(fmt::format("expected `{}` to be an Array", (*args[0])));
  const auto array = (ArrayBase*)args[0];  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  ASSERT(array);
  if (!scm::IsLong(args[1]))
    return ThrowError(fmt::format("expected `{}` to be a Long.", (*args[1])));
  const auto index = Long::Unbox(args[1]);
  if (index > array->GetCapacity())
    return ThrowError(fmt::format("index `{}` is out of bounds for `{}`", index, (const scm::Object&)*array));
  array->Set(index, args[2]);
  return DoNothing();
}

NATIVE_PROCEDURE_F(array_length) {
  ASSERT(HasRuntime());
  NativeArgument<0, ArrayBase> array(args);
  return ReturnNew<Long>(array->GetCapacity());
}

#ifdef SCM_DEBUG

NATIVE_PROCEDURE_F(scm_minor_gc) {
  scm::MinorCollection();
  return DoNothing();
}

NATIVE_PROCEDURE_F(scm_major_gc) {
  scm::MajorCollection();
  return DoNothing();
}

NATIVE_PROCEDURE_F(scm_get_debug) {
#ifdef SCM_DEBUG
  return Return(Bool::True());
#else
  return Return(Bool::False());
#endif  // SCM_DEBUG
}

NATIVE_PROCEDURE_F(scm_get_frame) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  DLOG(INFO) << "stack frames:";
  StackFrameIterator iter(runtime->GetStackFrames());
  while (iter.HasNext()) {
    DLOG(INFO) << "- " << iter.Next();
  }
  return DoNothing();
}

NATIVE_PROCEDURE_F(scm_get_locals) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  LocalScope::RecursiveIterator iter(GetRuntime()->GetCurrentFrame()->GetLocals());
  return Return(scm::ToList<LocalScope::RecursiveIterator, LocalVariable*>(iter, [](LocalVariable* local) -> Object* {
    return String::New(local->GetName());
  }));
}

NATIVE_PROCEDURE_F(scm_get_classes) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  ClassListIterator iter;
  return Return(scm::ToList(iter));
}

NATIVE_PROCEDURE_F(scm_get_target_triple) {
  ASSERT(HasRuntime());
  return ReturnNew<String>(SCM_TARGET_TRIPLE);
}

#endif  // SCM_DEBUG

#ifdef SCM_ENABLE_RX

#define NATIVE_RX_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(rx_##Name)

template <typename... Args>
static inline auto WrapPredicate(Runtime* runtime, Procedure* predicate) -> std::function<bool(Args...)> {
  ASSERT(runtime);
  return [runtime, predicate](Args... args) {
    runtime->Call(predicate->AsProcedure(), {args...});
    return scm::Truth(runtime->Pop());
  };
}

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
  source->Apply(rx::operators::filter(WrapPredicate<scm::Object*>(GetRuntime(), predicate)));
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
    return scm::ToList((const ObjectList&)values);
  });
  source->value_ = source->value_ | buffer | map;
  return DoNothing();
}

NATIVE_RX_PROCEDURE_F(get_operators) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  LocalScope::RecursiveIterator iter(rx::GetRxScope());
  return Return(scm::ToList<LocalScope::RecursiveIterator, LocalVariable*>(iter, [](LocalVariable* local) -> Object* {
    return String::New(local->GetName());
  }));
}

NATIVE_RX_PROCEDURE_F(observable) {
  // TODO: handle multiple args
  return ReturnNew<Observable>(args[0]);
}

// (rx:subscribe <observable> <on_next> <on_error?> <on_completed?>)
NATIVE_PROCEDURE_F(rx_subscribe) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (args.size() < 2 || args.size() > 4)
    return ThrowError(fmt::format("expected args to be: `<observable> <on_next> <on_error?> <on_completed?>`"));
  const auto source = args[0];
  if (!scm::IsObservable(source))
    return ThrowError(fmt::format("expected arg #1 `{}` to be an Observable", source->ToString()));
  const auto on_next = args[1];
  if (!scm::IsProcedure(on_next))
    return ThrowError(fmt::format("expected arg #2 `{}` to be a Procedure", *on_next));
  source->AsObservable()->GetValue().subscribe([runtime, on_next](Object* next) {
    return runtime->Call(on_next->AsProcedure(), ObjectList{next});
  });
  return DoNothing();
}

#define CHECK_ARG_TYPE(Index, Name, Type)                  \
  const auto Name = args[Index];                           \
  if (!(Name) || !(Name->GetType()->IsInstanceOf((Type)))) \
    return ThrowError(fmt::format("expected arg #{} ({}) `{}` to be a `{}`", Index, #Name, (*Name), ((Type)->GetName())->Get()));

// (rx:map <func>)
NATIVE_PROCEDURE_F(rx_map) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (args.size() != 2)
    return ThrowError(fmt::format("expected args to be: `<observable> <func>`"));
  CHECK_ARG_TYPE(0, source, Observable::GetClass());
  CHECK_ARG_TYPE(1, on_next, Procedure::GetClass());
  source->AsObservable()->Apply(rx::operators::map([on_next, runtime](Object* value) {
    runtime->Call(on_next->AsProcedure(), {value});
    return runtime->Pop();
  }));
  return DoNothing();
}

NATIVE_PROCEDURE_F(rx_take_while) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (args.size() != 2)
    return ThrowError(fmt::format("expected args to be: `<observable> <func>`"));
  CHECK_ARG_TYPE(0, source, Observable::GetClass());
  CHECK_ARG_TYPE(1, predicate, Procedure::GetClass());
  source->AsObservable()->Apply(rx::operators::take_while([predicate, runtime](Object* value) {
    runtime->Call(predicate->AsProcedure(), {value});
    return scm::Truth(runtime->Pop());
  }));
  return DoNothing();
}

#endif  // SCM_ENABLE_RX

}  // namespace scm::proc