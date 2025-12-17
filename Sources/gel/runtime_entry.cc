#include "gel/runtime_entry.h"

#include "gel/to_string_helper.h"

namespace gel {
auto RuntimeEntry::ToString() const -> std::string {
  ToStringHelper<RuntimeEntry> helper{};
  helper.AddField("body", GetBody());
  helper.AddField("code", GetCode());
  return helper;
}
}
