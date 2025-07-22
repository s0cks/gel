#include "gel/natives.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fmt/args.h>
#include <fmt/base.h>
#include <fmt/format.h>
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
#include "gel/platform.h"
#include "gel/procedure.h"
#include "gel/repl.h"
#include "gel/runtime.h"
#include "gel/rx.h"
#include "gel/shared_lib.h"
#include "gel/stack_frame.h"
#include "gel/timer.h"
#include "gel/type.h"
#include "gel/zone.h"

namespace gel {
#define INIT_GEL_NATIVE(Name) InitNative<gel_##Name>();

void NativeFn::InitNatives() {
  using namespace proc;
  INIT_GEL_NATIVE(get_version);
  INIT_GEL_NATIVE(sizeof);
  INIT_GEL_NATIVE(print);
  INIT_GEL_NATIVE(format);
  INIT_GEL_NATIVE(type);
  InitNative<import>();
  InitNative<exit>();
  InitNative<random>();
  InitNative<rand_range>();
  INIT_GEL_NATIVE(bit_str);
  INIT_GEL_NATIVE(on_shutdown);
  INIT_GEL_NATIVE(queue_utask);
  INIT_GEL_NATIVE(docs);
  INIT_GEL_NATIVE(load_bindings);
  INIT_GEL_NATIVE(get_event_loop);
  INIT_GEL_NATIVE(compare);

#define InitTimerNative(Name) InitNative<timer_##Name>()
  InitTimerNative(create);
  InitTimerNative(start);
  InitTimerNative(stop);
  InitTimerNative(again);
  InitTimerNative(get_due_in);
  InitTimerNative(get_repeat);
  InitTimerNative(set_repeat);
#undef InitTimerNative

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
  InitNative<gel_print_roots>();
  InitNative<gel_minor_gc>();
  InitNative<gel_major_gc>();
  InitNative<gel_get_debug>();
  InitNative<gel_get_target_triple>();
  InitNative<gel_get_locals>();
  InitNative<gel_get_natives>();
  InitNative<gel_get_compile_time>();
#endif  // GEL_DEBUG
}

namespace proc {
#define GEL_NATIVE_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(gel_##Name)

GEL_NATIVE_PROCEDURE_F(get_version) {
  return ReturnNew<Str>(gel::GetVersion());
}

GEL_NATIVE_PROCEDURE_F(sizeof) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  return ReturnNumber(value->GetType()->GetAllocationSize());
}

GEL_NATIVE_PROCEDURE_F(on_shutdown) {
  REQUIRED_NATIVE_ARG(0, Fn, callback);
  GetRuntime()->AddShutdownListener(*callback.GetValue());
  return ReturnNull();
}

GEL_NATIVE_PROCEDURE_F(queue_utask) {
  REQUIRED_NATIVE_ARG(0, Fn, task);
  GetThreadEventLoop()->AddTask(task);
  return ReturnNull();
}

GEL_NATIVE_PROCEDURE_F(compare) {
  REQUIRED_NATIVE_ARG(0, Object, x);
  REQUIRED_NATIVE_ARG(1, Object, y);
  return ReturnBool(x->Compare(y));
}

GEL_NATIVE_PROCEDURE_F(bit_str) {
  REQUIRED_NATIVE_ARG(0, Number, value);
  std::stringstream ss;
  ss << std::bitset<kWordSize>(value->AsRaw<uword>());
  return ReturnString(ss);
}

GEL_NATIVE_PROCEDURE_F(docs) {
  REQUIRED_NATIVE_ARG(0, Fn, func);
  if (func->IsLambdaFn()) {
    const auto lambda = func->AsLambdaFn();
    std::stringstream ss;
    if (lambda->HasSymbol())
      ss << lambda->GetSymbol()->GetFullyQualifiedName();
    ss << std::endl;
    ss << "([";
    const auto& args = lambda->GetArgs();
    if (args && !args->IsEmpty()) {
      for (word idx = static_cast<word>(args->GetLength() - 1); idx >= 0; idx--) {
        const auto arg = args->Get(idx);
        ss << arg->GetName()->Get();
        if (idx > 0)
          ss << ", ";
      }
    }
    ss << "])";
    ss << std::endl;
    ss << "  ";
    if (lambda->HasDocstring())
      ss << lambda->GetDocstring()->Get();
    return ReturnNew<Str>(ss.str());
  } else if (func->IsNative()) {
    const auto native = func->AsNativeFn();
    std::stringstream ss;
    ss << native->GetSymbol()->GetFullyQualifiedName() << std::endl;
    ss << "([";
    const auto& args = native->GetArgs();
    if (args && !args->IsEmpty()) {
      for (word idx = static_cast<word>(args->GetLength() - 1); idx >= 0; idx--) {
        const auto arg = args->Get(idx);
        ss << arg->GetName()->Get();
        if (idx > 0)
          ss << ", ";
      }
    }
    ss << "])";
    ss << std::endl;
    ss << "  ";
    if (native->HasDocstring())
      ss << native->GetDocstring()->Get();
    return ReturnNew<Str>(ss.str());
  }
  return ThrowError(fmt::format("`{}` is not a Fn", func->ToString()));
}

NATIVE_PROCEDURE_F(import) {
  REQUIRED_NATIVE_ARG(0, Symbol, symbol);
  if (!GetRuntime()->Import(symbol, GetRuntime()->GetScope())) {
    LOG(FATAL) << "failed to import module: " << symbol->ToString();
    return false;
  }
  DLOG(INFO) << symbol->ToString() << " imported!";
  return true;
}

GEL_NATIVE_PROCEDURE_F(print) {
  ASSERT(!args.empty());
#ifdef GEL_DEBUG
  if (VLOG_IS_ON(100))
    PrintValue(google::LogMessage(__FILE__, __LINE__, google::LogSeverity::INFO).stream(), args[0]);
#endif  // GEL_DEBUG
  PrintValue(std::cout, args[0]) << std::endl;
  return ReturnNull();
}

GEL_NATIVE_PROCEDURE_F(load_bindings) {
  REQUIRED_NATIVE_ARG(0, Str, filename);
  NativeBindings::Load(filename->Get()) | rx::operators::as_blocking() |
      rx::operators::subscribe([&filename](const int status) {
        LOG_IF(ERROR, status != EXIT_SUCCESS) << "failed to load bindings from " << filename->Get() << ": " << status;
      });
  return ReturnNull();
}

static std::random_device rand_device_{};   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static std::mt19937_64 mt(rand_device_());  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

NATIVE_PROCEDURE_F(random) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  return ReturnNumber(static_cast<RawNumber>(mt()));
}

NATIVE_PROCEDURE_F(rand_range) {
  ASSERT(HasRuntime());
  NativeArgument<0, Number> min(args);
  NativeArgument<1, Number> max(args);
  std::uniform_int_distribution<uword> distribution(min->AsRaw<uword>(), max->AsRaw<uword>());
  return ReturnNumber(distribution(mt));
}

GEL_NATIVE_PROCEDURE_F(type) {
  ASSERT(!args.empty());
  NativeArgument<0> value(args);
  if (value->IsNil())
    return ReturnNew<Str>("Null");
  return Return(value->GetType()->GetName());
}

NATIVE_PROCEDURE_F(exit) {
  // TODO: GetRuntime()->StopRunning();
  return true;
}

GEL_NATIVE_PROCEDURE_F(format) {
  REQUIRED_NATIVE_ARG(0, Str, format);
  const auto& fmt_val = format->Get();
  ASSERT(!fmt_val.empty());
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_args{};
  std::for_each(std::begin(args) + 1, std::end(args), [&fmt_args](Object* arg) {
    fmt_args.push_back(Str::ValueOf(arg)->Get());
  });
  const auto result = fmt::vformat(fmt_val, fmt_args);
  ASSERT(!result.empty());
  return ReturnNew<Str>(result);
}

GEL_NATIVE_PROCEDURE_F(get_event_loop) {
  return Return(GetThreadEventLoop());
}

#define OBJECT_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(object_##Name)

OBJECT_PROCEDURE_F(hashcode) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  return ReturnNumber(value->GetHashCode());
}

#undef OBJECT_PROCEDURE_F

#define TIMER_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(timer_##Name)

TIMER_PROCEDURE_F(start) {
  REQUIRED_NATIVE_ARG(0, Number, id);
  REQUIRED_NATIVE_ARG(1, Number, timeout_value);
  REQUIRED_NATIVE_ARG(2, Number, repeat);
  const auto timer = GetThreadEventLoop()->GetTimer(id->Get());
  if (!timer)  // TODO: create new Timer?
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  timer->Start(timeout_value->Get(), repeat->Get());
  return Return();
}

TIMER_PROCEDURE_F(stop) {
  REQUIRED_NATIVE_ARG(0, Number, id);
  const auto timer = GetThreadEventLoop()->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  timer->Stop();
  return Return();
}

TIMER_PROCEDURE_F(again) {
  REQUIRED_NATIVE_ARG(0, Number, id);
  const auto timer = GetThreadEventLoop()->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  timer->Again();
  return Return();
}

TIMER_PROCEDURE_F(get_repeat) {
  REQUIRED_NATIVE_ARG(0, Number, id);
  const auto timer = GetThreadEventLoop()->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  return ReturnNumber(timer->GetRepeat());
}

TIMER_PROCEDURE_F(set_repeat) {
  REQUIRED_NATIVE_ARG(0, Number, id);
  REQUIRED_NATIVE_ARG(1, Number, repeat);
  const auto timer = GetThreadEventLoop()->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  timer->SetRepeat(repeat->Get());
  return Return();
}

TIMER_PROCEDURE_F(get_due_in) {
  REQUIRED_NATIVE_ARG(0, Number, id);
  const auto timer = GetThreadEventLoop()->GetTimer(id->Get());
  if (!timer)
    return ThrowError(fmt::format("failed to find Timer w/ id {}", id->Get()));
  return ReturnNumber(timer->GetDueIn());
}

TIMER_PROCEDURE_F(create) {
  REQUIRED_NATIVE_ARG(0, Fn, on_tick);
  REQUIRED_NATIVE_ARG(1, Number, timeout_value);
  REQUIRED_NATIVE_ARG(2, Number, repeat);
  const auto timer = GetThreadEventLoop()->CreateTimer(on_tick);
  ASSERT(timer);
  timer->Start(timeout_value->Get(), repeat->Get());
  return ReturnNumber(timer->GetId());
}

#undef TIMER_PROCEDURE_F
}  // namespace proc
}  // namespace gel