#include "platform.h"

#include "common.h"

namespace gel::sys {
auto malloc(const uword sz) -> uword {
  ASSERT(sz > 0);
  return (uword)std::malloc(sz);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-no-malloc)
}

auto realloc(const uword ptr, const uword sz) -> uword {
  ASSERT(ptr);
  ASSERT(sz >= 0);
  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-pro-type-cstyle-cast)
  return (uword)std::realloc((void*)ptr, sz);
}

void free(const uword ptr) {
  ASSERT(ptr);
  return std::free((void*)ptr);  // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-pro-type-cstyle-cast)
}
}  // namespace gel::sys
