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
  const auto min = args[0];
  if (!min->IsLong())
    return ThrowError(fmt::format("expected min `{}` to be a Long", *min));
  const auto max = args[1];
  if (!max->IsLong())
    return ThrowError(fmt::format("expected max `{}` to be a Long", *max));
  std::uniform_int_distribution<uint64_t> distribution(Long::Unbox(min), Long::Unbox(max));
  return ReturnNew<Long>(distribution(mt));
}

NATIVE_PROCEDURE_F(type) {
  ASSERT(!args.empty());
  const auto value = args[0];
  ASSERT(value);
  if (value->IsPair() && value->AsPair()->IsEmpty())
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
  const auto format = args[0];
  ASSERT(format);
  if (!format->IsString())
    return ThrowError(fmt::format("expected {} to be a String.", *format));
  const auto& fmt_val = format->AsString()->Get();
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
  const auto& seq = args[0];
  ASSERT(seq);
  if (!seq->IsPair())
    return ThrowError(fmt::format("expected {} to be a Pair.", (*seq)));
  const auto& value = args[1];
  ASSERT(value);
  if (!value->IsDatum())
    return ThrowError(fmt::format("expected {} to be a Datum.", (*value)));
  SetCar(seq, value);
  return DoNothing();
}

NATIVE_PROCEDURE_F(set_cdr) {
  const auto& seq = args[0];
  ASSERT(seq);
  if (!seq->IsPair())
    return ThrowError(fmt::format("expected {} to be a Pair.", (*seq)));
  const auto& value = args[1];
  ASSERT(value);
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
  const auto value = Array<Object*>::New(length);
  ASSERT(value);
  for (auto idx = 0; idx < length; idx++) {
    ASSERT(args[idx]);
    value->Set(idx, args[idx]);
  }
  return Return(value);
}

NATIVE_PROCEDURE_F(array_get) {
  ASSERT(HasRuntime());
  if (args.size() != 2)
    return ThrowError(fmt::format("expected args `{}` to be: `<array> <index>`", Stringify(args)));
  if (!scm::IsArray(args[0]))
    return ThrowError(fmt::format("expected `{}` to be an Array", (*args[0])));
  const auto array = (ArrayBase*)args[0];  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  ASSERT(array);
  if (!scm::IsLong(args[1]))
    return ThrowError(fmt::format("expected `{}` to be a Long.", (*args[1])));
  const auto index = Long::Unbox(args[1]);
  if (index > array->GetCapacity())
    return ThrowError(fmt::format("index `{}` is out of bounds for `{}`", index, (const scm::Object&)*array));
  return Return(array->Get(index));
}

NATIVE_PROCEDURE_F(array_set) {
  ASSERT(HasRuntime());
  if (args.size() != 3)
    return ThrowError(fmt::format("expected args `{}` to be: `<array> <index>`", Stringify(args)));
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
  if (args.size() != 1)
    return ThrowError(fmt::format("expected args `{}` to be: `<array>`", Stringify(args)));
  if (!scm::IsArray(args[0]))
    return ThrowError(fmt::format("expected `{}` to be an Array", (*args[0])));
  const auto array = (ArrayBase*)args[0];  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  ASSERT(array);
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
  const auto frame = GetRuntime()->GetCurrentFrame();
  ASSERT(frame);
  LocalScope::RecursiveIterator iter(frame->GetLocals());
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

NATIVE_PROCEDURE_F(rx_get_locals) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  LocalScope::RecursiveIterator iter(rx::GetRxScope());
  return Return(scm::ToList<LocalScope::RecursiveIterator, LocalVariable*>(iter, [](LocalVariable* local) -> Object* {
    return String::New(local->GetName());
  }));
}

NATIVE_PROCEDURE_F(rx_to_observable) {
  // TODO: handle multiple args
  return ReturnNew<Observable>(args[0]);
}

// (rx:subscribe <observable> <on_next> <on_error?> <on_completed?>)
NATIVE_PROCEDURE_F(rx_subscribe) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (args.size() < 2 || args.size() > 4)
    return ThrowError(
        fmt::format("expected args `{}` to be: `<observable> <on_next> <on_error?> <on_completed?>`", Stringify(args)));
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

// (rx:map <func>)
NATIVE_PROCEDURE_F(rx_map) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (args.size() != 2)
    return ThrowError(fmt::format("expected args `{}` to be: `<observable> <func>`", Stringify(args)));
  const auto source = args[1];
  if (!scm::IsObservable(source))
    return ThrowError(fmt::format("expected arg #1 `{}` to be an Observable", *source));
  const auto on_next = args[0];
  if (!scm::IsProcedure(on_next))
    return ThrowError(fmt::format("expected arg 2 `{}` to be a Procedure", *on_next));
  DLOG(INFO) << "mapping " << source << " w/ " << on_next;
  source->AsObservable()->value_ = source->AsObservable()->value_ | rx::operators::map([](Object* value) {
                                     DLOG(INFO) << "- " << value;
                                     return value;
                                   });
  source->AsObservable()->value_.subscribe([](Object* value) {
    DLOG(INFO) << "- " << value;
  });
  return DoNothing();
}

#endif  // SCM_ENABLE_RX

}  // namespace scm::proc