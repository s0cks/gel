#include "gel/environment.h"

#include "gel/to_string_helper.h"

namespace gel {
auto Environment::ToString() const -> std::string {
  ToStringHelper<Environment> helper{};
  helper.AddField("parent", GetParent());
  return helper;
}
}  // namespace gel