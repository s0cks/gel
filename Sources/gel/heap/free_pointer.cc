#include "gel/free_pointer.h"

#include <string>

#include "gel/common.h"
#include "gel/region.h"
#include "gel/to_string_helper.h"

namespace gel {
auto FreePointer::ToString() const -> std::string {
  ToStringHelper<FreePointer> helper;
  helper.AddField("starting_address", (const void*)GetStartingAddressPointer());
  helper.AddField("tag", tag());
  helper.AddField("next", (const void*)GetNext());
  return helper;
}

auto FreePointer::Equals(const Region& rhs) const -> bool {
  return GetStartingAddress() == rhs.GetStartingAddress() && GetPointerSize() == rhs.GetSize();
}

auto FreePointer::Equals(FreePointer* rhs) const -> bool {
  ASSERT(rhs);
  return GetStartingAddress() == rhs->GetStartingAddress() &&
         GetTotalSize() == rhs->GetTotalSize();  // TODO: check tag()
}
}  // namespace gel