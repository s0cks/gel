#include "scheme/natives.h"

#include <fmt/args.h>
#include <fmt/base.h>
#include <fmt/format.h>

#include <iostream>

#include "scheme/error.h"
#include "scheme/local_scope.h"
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
  ASSERT(state);
  ASSERT(!args.empty());
  const auto arg = args[0];
  ASSERT(arg);
  const auto symbol = ToSymbol(arg);
  if (!symbol) {
    LOG(FATAL) << arg << " is not a valid Symbol.";
    return false;
  }

  if (!state->ImportModule((*symbol))) {
    LOG(FATAL) << "failed to import module: " << (*symbol);
    return false;
  }
  DLOG(INFO) << (*symbol) << " imported!";
  LOG(INFO) << "new scope:";
  LocalScopePrinter::Print<google::INFO, true>(state->GetScope(), __FILE__, __LINE__);
  return true;
}

NATIVE_PROCEDURE_F(print) {
  ASSERT(state);
  ASSERT(!args.empty());
  PrintValue(std::cout, args[0]) << std::endl;
  return true;
}

NATIVE_PROCEDURE_F(type) {
  ASSERT(state);
  ASSERT(!args.empty());
  const auto value = args[0];
  ASSERT(value);
  state->Push(String::New(value->GetTypename()));
  return true;
}

NATIVE_PROCEDURE_F(exit) {
  ASSERT(state);
  state->StopRunning();
  return true;
}

NATIVE_PROCEDURE_F(format) {
  ASSERT(state);
  ASSERT(args.size() >= 1);
  const auto format = args[0];
  ASSERT(format);
  if (!format->IsString()) {
    state->PushError(fmt::format("expected {} to be a String.", *format));
    return true;
  }

  const auto& fmt_val = format->AsString()->Get();
  ASSERT(!fmt_val.empty());
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_args{};
  std::for_each(std::begin(args) + 1, std::end(args), [&fmt_args](Type* arg) {
    fmt_args.push_back(arg->ToString());
  });
  const auto result = fmt::vformat(fmt_val, fmt_args);
  ASSERT(!result.empty());
  state->Push(String::New(result));
  return true;
}
}  // namespace scm::proc