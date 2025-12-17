#include "gel/to_string_helper.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <ostream>

#include "gel/common.h"

namespace gel::tostring {
void ToStringHelperBase::AddBytesField(const std::string_view name, const uword num_bytes) {
  ASSERT(!name.empty());
  std::stringstream ss{};
  ss << PrettyPrintBytes(num_bytes);
  return AddField(name, ss.str());
}

auto ToStringHelperBase::ToString() const -> std::string {
  std::stringstream ss{};
  ss << GetTypename() << GetChar(GetEnclosingStyle(), true);
  auto remaining = fields_.size();
  std::ranges::for_each(std::begin(fields_), std::end(fields_), [this, &ss, &remaining](const Field& field) {
    ss << field.name() << GetChar(GetValueSeparatorStyle()) << field.value();
    if (--remaining > 0)
      ss << GetChar(GetFieldSeparatorStyle()) << ' ';
  });
  ss << GetChar(GetEnclosingStyle(), false);
  return ss.str();
}
}  // namespace gel::tostring
