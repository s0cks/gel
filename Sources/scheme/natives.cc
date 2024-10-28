#include "scheme/natives.h"

#include <iostream>

#include "scheme/runtime.h"

namespace scm::proc {
print::print() :
  NativeProcedure(Symbol::New("print")) {}

auto print::Apply(Runtime* state) const -> bool {
  ASSERT(state);
  const auto value = state->Pop();
  ASSERT(value);
  PrintValue(std::cout, (*value)) << std::endl;
  return Null::Get();
}
}  // namespace scm::proc