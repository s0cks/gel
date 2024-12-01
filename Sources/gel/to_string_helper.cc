#include "gel/to_string_helper.h"

#include "gel/object.h"

namespace gel::tostring {
void ToStringHelperBase::AddField(const std::string& name, const gel::Object* value) {
  ASSERT(!name.empty());
  ASSERT(value);
  return AddField(name, value->ToString());
}

auto ToStringHelperBase::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetTypename() << GetChar(GetEncosingStyle(), true);
  auto remaining = fields_.size();
  std::ranges::for_each(std::begin(fields_), std::end(fields_), [this, &ss, &remaining](const Field& field) {
    ss << field.name() << GetChar(GetValueSeparatorStyle()) << field.value();
    if (--remaining > 0)
      ss << GetChar(GetFieldSeparatorStyle()) << ' ';
  });
  ss << GetChar(GetEncosingStyle(), false);
  return ss.str();
}
}  // namespace gel::tostring
