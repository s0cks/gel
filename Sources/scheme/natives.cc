#include "scheme/natives.h"

#include <iostream>

#include "scheme/runtime.h"
#include "scheme/type.h"

namespace scm::proc {
NATIVE_PROCEDURE_F(print) {
  ASSERT(state);
  const auto value = state->Pop();
  ASSERT(value);
  PrintValue(std::cout, (*value)) << std::endl;
  return true;
}

NATIVE_PROCEDURE_F(type) {
  ASSERT(state);
  const auto value = state->Pop();
  ASSERT(value);
  state->Push(String::New((*value)->GetTypename()));
  return true;
}
}  // namespace scm::proc