#include "gel/natives.h"

#include <fmt/args.h>
#include <fmt/base.h>
#include <fmt/format.h>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <random>
#include <ranges>
#include <rpp/observables/dynamic_observable.hpp>
#include <rpp/operators/fwd.hpp>
#include <rpp/operators/subscribe.hpp>
#include <rpp/sources/create.hpp>

#include "gel/argument.h"
#include "gel/array.h"
#include "gel/buffer.h"
#include "gel/collector.h"
#include "gel/common.h"
#include "gel/error.h"
#include "gel/event_loop.h"
#include "gel/gel.h"
#include "gel/heap.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/module_loader.h"
#include "gel/native_bindings.h"
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
  InitNative<gel_get_version>();
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
  InitNative<gel_load_bindings>();
  InitNative<get_event_loop>();

  InitNative<get_classes>();
  InitNative<get_class>();

  InitNative<get_namespace>();
  InitNative<ns_get>();

  InitNative<create_timer>();
#define InitTimerNative(Name) InitNative<timer_##Name>()
  InitTimerNative(start);
  InitTimerNative(stop);
  InitTimerNative(again);
  InitTimerNative(get_due_in);
  InitTimerNative(get_repeat);
  InitTimerNative(set_repeat);
#undef InitTimerNative

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

// TODO: add sandbox switch
#define InitFsNative(Name) InitNative<fs_##Name>();
  InitFsNative(get_cwd);
  InitFsNative(stat);
  InitFsNative(rename);
  InitFsNative(mkdir);
  InitFsNative(rmdir);
  InitFsNative(fsync);
  InitFsNative(ftruncate);
  InitFsNative(access);
  InitFsNative(chmod);
  InitFsNative(link);
  InitFsNative(symlink);
  InitFsNative(readlink);
  InitFsNative(chown);
  InitFsNative(copy_file);
  InitFsNative(open);
  InitFsNative(close);
  InitFsNative(unlink);
#undef InitFsNative

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
  InitNative<gel_print_args>();
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
  InitNative<gel_get_natives>();
  InitNative<gel_get_compile_time>();
  InitNative<gel_print_st>();
#endif  // GEL_DEBUG
}

namespace proc {
NATIVE_PROCEDURE_F(gel_get_version) {
  return ReturnNew<String>(gel::GetVersion());
}

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
      ss << lambda->GetSymbol()->GetFullyQualifiedName();
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
    ss << native->GetSymbol()->GetFullyQualifiedName() << std::endl;
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

NATIVE_PROCEDURE_F(gel_load_bindings) {
  NativeArgument<0, String> filename(args);
  if (!filename)
    return Throw(filename.GetError());
  NativeBindings::Load(filename->Get()) | rx::operators::as_blocking() | rx::operators::subscribe([&filename](const int status) {
    LOG_IF(ERROR, status != EXIT_SUCCESS) << "failed to load bindings from " << filename->Get() << ": " << status;
  });
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

NATIVE_PROCEDURE_F(get_classes) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  Object* result = Null();
  const auto visitor = [&result](Class* cls) {
    result = Cons(cls, result);
    return true;
  };
  LOG_IF(FATAL, !Class::VisitClasses(visitor, true)) << "failed to visit classes.";
  return Return(result);
}

NATIVE_PROCEDURE_F(get_class) {
  NativeArgument<0, Symbol> symbol(args);
  if (!symbol)
    return Throw(symbol.GetError());
  return Return(Class::FindClass(symbol));
}

NATIVE_PROCEDURE_F(get_namespace) {
  NativeArgument<0, Symbol> symbol(args);
  if (!symbol)
    return Throw(symbol.GetError());
  return Return(Namespace::FindNamespace(symbol));
}

NATIVE_PROCEDURE_F(ns_get) {
  NativeArgument<0> symOrNs(args);
  if (!symOrNs)
    return Throw(symOrNs.GetError());
  const auto ns = symOrNs->IsSymbol() ? Namespace::FindNamespace(symOrNs->AsSymbol()) : symOrNs->AsNamespace();
  ASSERT(ns);
  NativeArgument<1, Symbol> symbol(args);
  if (!symbol)
    return Throw(symbol.GetError());
  return Return(ns->Get(symbol));
}

NATIVE_PROCEDURE_F(get_event_loop) {
  return Return(GetThreadEventLoop());
}

#define TIMER_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(timer_##Name)

TIMER_PROCEDURE_F(start) {
  NativeArgument<0, Long> id(args);
  if (!id)
    return Throw(id.GetError());
  NativeArgument<1, Long> timeout(args);
  if (!timeout)
    return Throw(timeout.GetError());
  NativeArgument<2, Long> repeat(args);
  if (!repeat)
    return Throw(repeat.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  const auto timer = loop->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  timer->Start(timeout->Get(), repeat->Get());
  return Return();
}

TIMER_PROCEDURE_F(stop) {
  NativeArgument<0, Long> id(args);
  if (!id)
    return Throw(id.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  const auto timer = loop->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  timer->Stop();
  return Return();
}

TIMER_PROCEDURE_F(again) {
  NativeArgument<0, Long> id(args);
  if (!id)
    return Throw(id.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  const auto timer = loop->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  timer->Again();
  return Return();
}

TIMER_PROCEDURE_F(get_repeat) {
  NativeArgument<0, Long> id(args);
  if (!id)
    return Throw(id.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  const auto timer = loop->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  return ReturnNew<Long>(timer->GetRepeat());
}

TIMER_PROCEDURE_F(set_repeat) {
  NativeArgument<0, Long> id(args);
  if (!id)
    return Throw(id.GetError());
  NativeArgument<1, Long> repeat(args);
  if (!repeat)
    return Throw(repeat.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  const auto timer = loop->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  timer->SetRepeat(repeat->Get());
  return Return();
}

TIMER_PROCEDURE_F(get_due_in) {
  NativeArgument<0, Long> id(args);
  if (!id)
    return Throw(id.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  const auto timer = loop->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  return ReturnNew<Long>(timer->GetDueIn());
}

#undef TIMER_PROCEDURE_F

NATIVE_PROCEDURE_F(create_timer) {
  NativeArgument<0, Procedure> on_tick(args);
  if (!on_tick)
    return Throw(on_tick.GetError());
  NativeArgument<1, Long> timeout(args);
  if (!timeout)
    return Throw(timeout.GetError());
  NativeArgument<2, Long> repeat(args);
  if (!repeat)
    return Throw(repeat.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  const auto timer = loop->CreateTimer(on_tick);
  ASSERT(timer);
  timer->Start(timeout->Get(), repeat->Get());
  return ReturnNew<Long>(timer->GetId());
}
}  // namespace proc
}  // namespace gel