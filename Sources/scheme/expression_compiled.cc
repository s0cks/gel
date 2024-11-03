#include "scheme/expression_compiled.h"

#include "scheme/runtime.h"

namespace scm {
auto CompiledExpression::Apply(Runtime* state) -> bool {
  ASSERT(state);
  ASSERT(HasEntry());
  return state->Execute(GetEntry());
}

auto CompiledExpression::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsCompiledExpression())
    return false;
  return GetEntry() == rhs->AsCompiledExpression()->GetEntry();
}

auto CompiledExpression::ToString() const -> std::string {
  std::stringstream ss;
  ss << "CompiledExpression(";
  if (HasEntry())
    ss << "entry=" << GetEntry();
  ss << ")";
  return ss.str();
}
}  // namespace scm