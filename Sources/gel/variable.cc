#include "gel/variable.h"

#include <sstream>
#include <string>

#include "gel/to_string_helper.h"

namespace gel {
auto Variable::ToString() const -> std::string {
  ToStringHelper<Variable> helper{};
  helper.AddField("name", GetName());
  helper.AddField("value", GetValue());
  return helper;
}
}  // namespace gel