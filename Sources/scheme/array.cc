#include "scheme/array.h"

#include "scheme/common.h"

namespace scm {
auto ArrayBase::VisitPointers(const std::function<bool(Pointer**)>& vis) -> bool {
  ArrayPointerIterator iter(this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    if (!IsUnallocated(next)) {
      if (!vis(next))
        return false;
    }
  }
  return true;
}

void ArrayBase::Resize(const uword cap) {
  if (cap > GetCapacity()) {
    const auto new_cap = RoundUpPow2(static_cast<word>(cap));
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-pro-type-cstyle-cast)
    const auto new_data = (uword*)realloc(data_, new_cap * sizeof(uword));
    LOG_IF(FATAL, !new_data) << "failed to resize Array to: " << new_cap;
    data_ = new_data;
    capacity_ = new_cap;
  }
  length_ = cap;
}
}  // namespace scm