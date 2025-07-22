#include "gel/type/nil.h"

#include "gel/allocator.h"
#include "gel/common.h"
#include "gel/heap.h"
#include "gel/os_thread.h"
#include "gel/thread_local.h"
#include "gel/to_string_helper.h"

namespace gel {
static LazyThreadLocal<Nil> instance_([]() -> Nil* {
  return Nil::New();
});

auto Nil::operator new(const size_t sz) -> void* {
  ASSERT(CurrentThreadHasHeap());
  const auto new_address = GetCurrentThreadHeap()->TryAllocate(sz);
  LOG_IF(FATAL, IsUnallocated(new_address)) << "failed to allocate memory for new nil instance.";
  return reinterpret_cast<void*>(new_address);
}

auto Nil::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= 0xBAB3;
  return hash;
}

auto Nil::ToString() const -> std::string {
  ToStringHelper<Nil> helper{};
  return helper;
}

auto Nil::Equals(Value* rhs) const -> bool {
  return rhs && rhs->IsNil();
}

auto Nil::Compare(Value* rhs) const -> int {
  NOT_IMPLEMENTED(ERROR);  // TODO: @s0cks implement
  return -1;
}

auto Nil::Get() -> Nil* {
  return instance_;
}

}  // namespace gel