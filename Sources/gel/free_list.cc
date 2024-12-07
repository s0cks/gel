#include "gel/free_list.h"

#include "gel/common.h"
#include "gel/platform.h"
#include "gel/to_string_helper.h"

namespace gel {
auto FreePointer::ToString() const -> std::string {
  ToStringHelper<FreePointer> helper;
  helper.AddField("starting_address", (const void*)GetStartingAddressPointer());
  helper.AddField("tag", tag());
  helper.AddField("next", (const void*)GetNext());
  return helper;
}

auto operator<<(std::ostream& stream, const FreePointer& rhs) -> std::ostream& {
  return stream << rhs.ToString();
}

auto FreePointer::Equals(FreePointer* rhs) const -> bool {
  ASSERT(rhs);
  return GetStartingAddress() == rhs->GetStartingAddress() && GetTotalSize() == rhs->GetTotalSize();  // TODO: check tag()
}

auto FreeList::VisitFreePointers(const std::function<bool(FreePointer*)>& vis) const -> bool {
  auto current = head_;
  while (current != nullptr) {
    if (!vis(current))
      return false;
    current = current->GetNext();
  }
  return true;
}

auto FreeList::TryAllocate(const uword size) -> uword {
  ASSERT(size > UNALLOCATED);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return UNALLOCATED;
}
}  // namespace gel