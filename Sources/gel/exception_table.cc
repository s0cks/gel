#include "exception_table.h"

#include "to_string_helper.h"

namespace gel {
auto ExceptionTable::Entry::ToString() const -> std::string {
  ToStringHelper<Entry> helper{};
  helper.AddField("start", start);
  helper.AddField("finish", finish);
  helper.AddField("target", target);
  return helper;
}

auto ExceptionTable::ToString() const -> std::string {
  ToStringHelper<ExceptionTable> helper{};
  helper.AddArrayField("entries", entries_);
  return helper;
}
}  // namespace gel
