#include "scheme/natives.h"

#include <fmt/args.h>
#include <fmt/base.h>
#include <fmt/format.h>

#include <iostream>

#include "scheme/argument.h"
#include "scheme/error.h"
#include "scheme/local_scope.h"
#include "scheme/native_procedure.h"
#include "scheme/procedure.h"
#include "scheme/runtime.h"
#include "scheme/type.h"

namespace scm::proc {
static inline auto ToSymbol(Type* rhs) -> std::optional<Symbol*> {
  if (!rhs)
    return std::nullopt;
  if (rhs->IsSymbol())
    return {rhs->AsSymbol()};
  else if (rhs->IsString())
    return {Symbol::New(rhs->AsString()->Get())};
  return std::nullopt;
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
  if (!GetRuntime()->ImportModule((*symbol))) {
    LOG(FATAL) << "failed to import module: " << (*symbol);
    return false;
  }
  DLOG(INFO) << (*symbol) << " imported!";
  LOG(INFO) << "new scope:";
  LocalScopePrinter::Print<google::INFO, true>(GetRuntime()->GetScope(), __FILE__, __LINE__);
  return true;
}

NATIVE_PROCEDURE_F(print) {
  ASSERT(!args.empty());
  PrintValue(std::cout, args[0]) << std::endl;
  return true;
}

NATIVE_PROCEDURE_F(type) {
  ASSERT(!args.empty());
  const auto value = args[0];
  ASSERT(value);
  return ReturnValue(String::New(value->GetTypename()));
}

NATIVE_PROCEDURE_F(exit) {
  GetRuntime()->StopRunning();
  return true;
}

NATIVE_PROCEDURE_F(list) {
  if (args.empty())
    return Null::Get();
  Type* result = Null::Get();
  for (const auto& arg : args) {
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
  std::for_each(std::begin(args) + 1, std::end(args), [&fmt_args](Type* arg) {
    fmt_args.push_back(String::ValueOf(arg)->Get());
  });
  const auto result = fmt::vformat(fmt_val, fmt_args);
  ASSERT(!result.empty());
  return ReturnValue(String::New(result));
}

NATIVE_PROCEDURE_F(foreach) {
  ASSERT(GetRuntime());
  if (args.size() != 2)
    return ThrowError("expected: (foreach func list)");
  const auto f = args[0];
  ASSERT(f);
  if (!f->IsProcedure())
    return ThrowError(fmt::format("expected {} to be a procedure.", f->ToString()));
  DLOG(INFO) << "proc: " << f->ToString();

  const auto values = args[1];
  auto value = values;
  do {
    const auto result = GetRuntime()->Apply(f->AsProcedure(), {value});
    DLOG_IF(INFO, result) << "result: " << result->ToString();
    if (result)
      return ReturnValue(result);
  } while (true);
  return ThrowError(fmt::format("{} is not implemented!", __PRETTY_FUNCTION__));
}

NATIVE_PROCEDURE_F(map) {
  ASSERT(GetRuntime());
  if (args.size() != 2)
    return ThrowError("expected: (foreach func list)");
  const auto f = args[0];
  ASSERT(f);
  if (!f->IsProcedure())
    return ThrowError(fmt::format("expected {} to be a procedure.", f->ToString()));
  DLOG(INFO) << "proc: " << f->ToString();

  auto result = Symbol::New("()");
  ASSERT(result);

  const auto values = args[1];
  auto value = values;
  do {
    const auto rhs = GetRuntime()->Apply(f->AsProcedure(), {value});
    ASSERT(rhs);
  } while (true);
  return ReturnValue(result);
}

#ifdef SCM_DEBUG

NATIVE_PROCEDURE_F(list_symbols) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());

  const auto scope = GetRuntime()->GetScope();
  ASSERT(scope);

  LocalScopePrinter::Print(scope, __FILE__, __LINE__);
  return true;
}

#endif  // SCM_DEBUG
}  // namespace scm::proc