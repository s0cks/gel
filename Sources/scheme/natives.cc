#include "scheme/natives.h"

#include <fmt/args.h>
#include <fmt/base.h>
#include <fmt/format.h>

#include <iostream>
#include <random>
#include <ranges>

#include "scheme/argument.h"
#include "scheme/collector.h"
#include "scheme/common.h"
#include "scheme/error.h"
#include "scheme/local_scope.h"
#include "scheme/native_procedure.h"
#include "scheme/object.h"
#include "scheme/parser.h"
#include "scheme/procedure.h"
#include "scheme/runtime.h"
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
  return true;
}

static std::random_device rand_device_{};   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static std::mt19937_64 mt(rand_device_());  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

NATIVE_PROCEDURE_F(random) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  return ReturnValue(Long::New(mt()));
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
  return ReturnValue(Long::New(distribution(mt)));
}

NATIVE_PROCEDURE_F(type) {
  ASSERT(!args.empty());
  const auto value = args[0];
  ASSERT(value);
  if (value->IsPair() && value->AsPair()->IsEmpty())
    return ReturnValue(String::New("Null"));
  return ReturnValue(value->GetType()->GetName());
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
  return ReturnValue(result);
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
  return ReturnValue(String::New(result));
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
  return true;
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
  return true;
}

#ifdef SCM_DEBUG

NATIVE_PROCEDURE_F(minor_gc) {
  scm::MinorCollection();
  return true;
}

NATIVE_PROCEDURE_F(frame) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  DLOG(INFO) << "stack frames:";
  StackFrameIterator iter(runtime->GetStackFrames());
  while (iter.HasNext()) {
    DLOG(INFO) << "- " << iter.Next();
  }
  return true;
}

NATIVE_PROCEDURE_F(list_symbols) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  const auto frame = GetRuntime()->GetCurrentFrame();
  ASSERT(frame);
  const auto locals = frame->GetLocals();
  ASSERT(locals);
  LOG(INFO) << "Locals:";
  LocalScopePrinter::Print<google::INFO, true>(locals, __FILE__, __LINE__);
  return true;
}

NATIVE_PROCEDURE_F(list_classes) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  DLOG(INFO) << "classes: ";
  for (const auto& cls : Class::GetAllClasses()) {
    ASSERT(cls);
    DLOG(INFO) << "- " << cls->ToString();
  }
  return true;
}

#endif  // SCM_DEBUG
}  // namespace scm::proc