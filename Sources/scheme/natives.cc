#include "scheme/natives.h"

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
  const auto arg = state->Pop();
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
  const auto value = state->Pop();
  ASSERT(value);
  PrintValue(std::cout, value) << std::endl;
  return true;
}

NATIVE_PROCEDURE_F(throw_exc) {
  ASSERT(state);
  const auto message = state->Pop();
  ASSERT(message && message->IsString());
  state->Push(Error::New(message));
  return true;
}

NATIVE_PROCEDURE_F(type) {
  ASSERT(state);
  const auto value = state->Pop();
  ASSERT(value);
  state->Push(String::New(value->GetTypename()));
  return true;
}

NATIVE_PROCEDURE_F(exit) {
  ASSERT(state);
  state->StopRunning();
  return true;
}
}  // namespace scm::proc