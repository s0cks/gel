#include "gel/natives.h"

#include <fmt/args.h>
#include <fmt/base.h>
#include <fmt/format.h>

#include <cstdlib>
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
#include "gel/heap.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/parser.h"
#include "gel/procedure.h"
#include "gel/runtime.h"
#include "gel/rx.h"
#include "gel/shared_lib.h"
#include "gel/stack_frame.h"
#include "gel/type.h"
#include "gel/zone.h"

namespace gel {
void NativeProcedure::InitNatives() {
  using namespace proc;
  InitNative<hashcode>();
  InitNative<gel_sizeof>();
  InitNative<print>();
  InitNative<type>();
  InitNative<import>();
  InitNative<exit>();
  InitNative<format>();
  InitNative<list>();
  InitNative<set_car>();
  InitNative<set_cdr>();
  InitNative<random>();
  InitNative<rand_range>();
  InitNative<array_new>();
  InitNative<array_get>();
  InitNative<array_set>();
  InitNative<array_length>();
  InitNative<gel_docs>();
  InitNative<dlopen>();

#define InitSetNative(Name) InitNative<set_##Name>()
  InitSetNative(contains);
  InitSetNative(empty);
  InitSetNative(count);
#undef InitSetNative

#define InitMapNative(Name) InitNative<map_##Name>()
  InitMapNative(contains);
  InitMapNative(empty);
  InitMapNative(size);
  InitMapNative(get);
#undef InitMapNative

#ifdef GEL_ENABLE_RX
#define REGISTER_RX(Name) InitNative<rx_##Name>();
  REGISTER_RX(observer);
  REGISTER_RX(observable);
  REGISTER_RX(subscribe);
  REGISTER_RX(first);
  REGISTER_RX(last);
  REGISTER_RX(map);
  REGISTER_RX(take);
  REGISTER_RX(take_last);
  REGISTER_RX(skip);
  REGISTER_RX(buffer);
  REGISTER_RX(filter);
  REGISTER_RX(take_while);
  REGISTER_RX(replay_subject);
  REGISTER_RX(publish_subject);
  REGISTER_RX(publish);
  REGISTER_RX(complete);
  REGISTER_RX(publish_error);
#undef REGISTER_RX
#endif  // GEL_ENABLE_RX

#ifdef GEL_DEBUG
  InitNative<gel_print_heap>();
  InitNative<gel_print_new_zone>();
  InitNative<gel_print_old_zone>();
  InitNative<gel_get_roots>();
  InitNative<gel_minor_gc>();
  InitNative<gel_major_gc>();
  InitNative<gel_get_frame>();
  InitNative<gel_get_debug>();
  InitNative<gel_get_target_triple>();
  InitNative<gel_get_locals>();
  InitNative<gel_get_classes>();
  InitNative<gel_get_natives>();
  InitNative<gel_get_compile_time>();
  InitNative<gel_print_st>();
#endif  // GEL_DEBUG
}

namespace proc {
NATIVE_PROCEDURE_F(hashcode) {
  ASSERT(args.size() == 1);
  NativeArgument<0> value(args);
  if (!value)
    return Throw(value.GetError());
  return ReturnNew<Long>(value->HashCode());
}

NATIVE_PROCEDURE_F(gel_sizeof) {
  ASSERT(args.size() == 1);
  NativeArgument<0> value(args);
  if (!value)
    return Throw(value.GetError());
  return ReturnNew<Long>(value->GetType()->GetAllocationSize());
}

NATIVE_PROCEDURE_F(gel_docs) {
  if (args.empty())
    return DoNothing();
  OptionalNativeArgument<0, Procedure> func(args);
  if (!func)
    return Throw(func.GetError());
  if (func->IsLambda()) {
    const auto lambda = func->AsLambda();
    std::stringstream ss;
    if (lambda->HasSymbol())
      ss << lambda->GetSymbol()->Get();
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
  if (!GetRuntime()->Import(symbol, GetRuntime()->GetScope())) {
    LOG(FATAL) << "failed to import module: " << symbol;
    return false;
  }
  DLOG(INFO) << symbol << " imported!";
  return true;
}

NATIVE_PROCEDURE_F(print) {
  ASSERT(!args.empty());
  PrintValue(std::cout, args[0]) << std::endl;
  return ReturnNull();
}

NATIVE_PROCEDURE_F(dlopen) {
  NativeArgument<0, String> filename(args);
  if (!filename)
    return Throw(filename.GetError());
  SharedLibrary lib(filename->Get());
  const auto get_plugin_name = lib.DlSym<PluginGetNameFunc>("GetPluginName");
  const auto name = get_plugin_name();
  DLOG(INFO) << "initializing " << name << " plugin....";
  // TODO: better lib open error detection
  const auto init_plugin = lib.DlSym<PluginInitCallback>("InitPlugin");
  const auto status = init_plugin();
  if (status != EXIT_SUCCESS) {
    LOG(ERROR) << "failed to initialize the " << name << " plugin: " << status;
    return ReturnLong(status);
  }
  return ReturnNull();
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
  // TODO: GetRuntime()->StopRunning();
  return true;
}

#define SET_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(set_##Name)

SET_PROCEDURE_F(contains) {
  NativeArgument<0, Set> set(args);
  if (!set)
    return Throw(set.GetError());
  NativeArgument<1> value(args);
  if (!value)
    return Throw(value.GetError());
  return Return(Bool::Box(set->Contains(value)));
}

SET_PROCEDURE_F(count) {
  NativeArgument<0, Set> set(args);
  if (!set)
    return Throw(set.GetError());
  return ReturnNew<Long>(set->GetSize());
}

SET_PROCEDURE_F(empty) {
  NativeArgument<0, Set> set(args);
  if (!set)
    return Throw(set.GetError());
  return Return(Bool::Box(set->IsEmpty()));
}

#undef SET_PROCEDURE_F

#define MAP_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(map_##Name)
MAP_PROCEDURE_F(contains) {
  NativeArgument<0, Map> m(args);
  if (!m)
    return Throw(m.GetError());
  NativeArgument<1> key(args);
  if (!key)
    return Throw(key.GetError());
  return ReturnBool(m->Contains(key));
}

MAP_PROCEDURE_F(get) {
  NativeArgument<0, Map> m(args);
  if (!m)
    return Throw(m.GetError());
  NativeArgument<1> key(args);
  if (!key)
    return Throw(key.GetError());
  return Return(m->Get(key));
}

MAP_PROCEDURE_F(size) {
  NativeArgument<0, Map> m(args);
  if (!m)
    return Throw(m.GetError());
  return ReturnLong(m->GetSize());
}

MAP_PROCEDURE_F(empty) {
  NativeArgument<0, Map> m(args);
  if (!m)
    return Throw(m.GetError());
  return ReturnBool(m->IsEmpty());
}
#undef MAP_PROCEDURE_F

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
  if (!seq)
    return Throw(seq.GetError());
  NativeArgument<1> value(args);
  if (!value)
    return Throw(value.GetError());
  SetCar(seq, value);
  return DoNothing();
}

NATIVE_PROCEDURE_F(set_cdr) {
  NativeArgument<0, Pair> seq(args);
  if (!seq)
    return Throw(seq.GetError());
  NativeArgument<1> value(args);
  if (!value)
    return Throw(value.GetError());
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

NATIVE_PROCEDURE_F(gel_print_heap) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return Return();
}

NATIVE_PROCEDURE_F(gel_print_new_zone) {
  const auto heap = Heap::GetHeap();
  if (!heap)
    return Return();
  PrintNewZone(heap->GetNewZone());
  return Return();
}

NATIVE_PROCEDURE_F(gel_print_old_zone) {
  const auto heap = Heap::GetHeap();
  if (!heap)
    return Return();
  PrintOldZone(heap->GetOldZone());
  return Return();
}

NATIVE_PROCEDURE_F(gel_get_roots) {
  Object* result = Null();
  LOG_IF(FATAL, !VisitRoots([&result](Pointer** ptr) {
           ASSERT((*ptr));
           result = Pair::New((*ptr)->GetObjectPointer(), result);
           return true;
         }))
      << "failed to visit roots.";
  return Return(result);
}

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
  StackFrameIterator iter(runtime->stack_);
  while (iter.HasNext()) {
    DLOG(INFO) << "- " << iter.Next();
  }
  return DoNothing();
}

NATIVE_PROCEDURE_F(gel_print_st) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  LOG(INFO) << "Stack Trace:";
  StackFrameIterator iter(runtime->stack_);
  while (iter.HasNext()) {
    const auto& next = iter.Next();
    LOG(INFO) << "  " << next.GetId() << ": " << next.GetTargetName();
  }
  return DoNothing();
}

NATIVE_PROCEDURE_F(gel_get_locals) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  LocalScope::Iterator iter(GetRuntime()->GetScope());
  return Return(gel::ToList<LocalScope::Iterator, LocalVariable*>(iter, [](LocalVariable* local) -> Object* {
    return gel::ToList(ObjectList{

        local->HasValue() ? local->GetValue() : Null(),
        String::New(local->GetName()),
    });
  }));
}

NATIVE_PROCEDURE_F(gel_get_classes) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  Object* result = Null();
  const auto visitor = [&result](Class* cls) {
    ObjectList meta;
    meta.push_back(cls->GetName());
    if (cls->HasParent())
      meta.push_back(cls->GetParent()->GetName());
    result = Pair::New(gel::ToList((const ObjectList&)meta, true), result);
    return true;
  };
  LOG_IF(FATAL, !Class::VisitClasses(visitor, true)) << "failed to visit classes.";
  return Return(result);
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
  return ReturnNew<Long>(lambda->GetCompileTime());
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

}  // namespace proc
}  // namespace gel