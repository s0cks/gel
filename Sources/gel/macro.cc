#include "gel/macro.h"

#include <sstream>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"

namespace gel {
auto Macro::New(const ObjectList& args) -> Macro* {
  NOT_IMPLEMENTED(FATAL);
}

auto Macro::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Macro");
}

auto Macro::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsMacro())
    return false;
  const auto other = rhs->AsMacro();
  ASSERT(other);
  return GetSymbol()->Equals(other->GetSymbol());
}

auto Macro::ToString() const -> std::string {
  ToStringHelper<Macro> helper;
  helper.AddField("symbol", GetSymbol()->Get());
  if (IsEmpty())
    helper.AddField("empty", IsEmpty());
  return helper;
}
}  // namespace gel