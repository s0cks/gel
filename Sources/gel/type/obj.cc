#include "gel/type/obj.h"

#include "gel/common.h"
#include "gel/to_string_helper.h"

namespace gel {
auto Obj::ToString() const -> std::string {
  ToStringHelper<Obj> helper{};
  helper.AddField("parent", GetParent());
  return helper;
}

auto Obj::GetHashCode() const -> HashCode {
  HashCode hash{};
  if (HasParent())
    hash ^= GetParent()->GetHashCode();
  return hash;
}

auto Obj::Compare(Value* rhs) const -> std::strong_ordering {
  return std::strong_ordering::equivalent;
}

auto Obj::GetProperty(Str* name) const -> Value* {
  ASSERT(name);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement @s0cks
  return nullptr;
}

auto Obj::PutProperty(Str* name, Value* value) -> bool {
  ASSERT(name);
  ASSERT(value);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement @s0cks
  return false;
}
}  // namespace gel
