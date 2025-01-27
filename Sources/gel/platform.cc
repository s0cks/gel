#include "gel/platform.h"

#include "gel/common.h"

namespace gel::sys {
auto malloc(const uword sz) -> uword {
  ASSERT(sz > 0);
  return (uword)std::malloc(sz);  // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-pro-type-cstyle-cast)
}

auto realloc(const uword ptr, const uword sz) -> uword {
  ASSERT(ptr);
  ASSERT(sz >= 0);
  return (uword)std::realloc((void*)ptr, sz);  // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-pro-type-cstyle-cast)
}

void free(const uword ptr) {
  ASSERT(ptr);
  return std::free((void*)ptr);  // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-pro-type-cstyle-cast)
}
}  // namespace gel::sys