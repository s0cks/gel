#include "gel/type/nil.h"
#include <compare>

#include "gel/common.h"
#include "gel/os_thread.h"
#include "gel/thread_local.h"
#include "gel/to_string_helper.h"

#ifdef GEL_ENABLE_HEAP
#include "gel/heap/heap.h"
#include "gel/heap/allocator.h"
#endif //GEL_ENABLE_HEAP

namespace gel {
static LazyThreadLocal<Nil> instance_([]() -> Nil* {
  return Nil::New();
});

#ifdef GEL_ENABLE_HEAP
auto Nil::operator new(const size_t sz) -> void* {
  ASSERT(CurrentThreadHasHeap());
  const auto new_address = GetCurrentThreadHeap()->TryAllocate(sz);
  LOG_IF(FATAL, IsUnallocated(new_address)) << "failed to allocate memory for new nil instance.";
  return reinterpret_cast<void*>(new_address);
}
#else

auto Nil::operator new(const size_t sz) -> void* {
  return reinterpret_cast<void*>(sys::malloc(sz));
}

#endif //GEL_ENABLE_HEAP

auto Nil::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= 0xBAB3;
  return hash;
}

auto Nil::ToString() const -> std::string {
  ToStringHelper<Nil> helper{};
  return helper;
}

auto Nil::Compare(Value* rhs) const -> std::strong_ordering {
  NOT_IMPLEMENTED(ERROR);  // TODO: @s0cks implement
  return std::strong_ordering::equivalent;
}

auto Nil::Get() -> Nil* {
  return instance_;
}

}  // namespace gel
