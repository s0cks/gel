#include "compiled_code.h"

#include <cstddef>
#include <string>

#include "common.h"
#include "heap.h"
#include "platform.h"
#include "to_string_helper.h"

namespace gel {
#ifdef GEL_DISABLE_HEAP

auto CompiledCode::operator new(const size_t sz) -> void* {
  return malloc(sz);
}

void CompiledCode::operator delete(void* ptr) {
  ASSERT(ptr);
  free(ptr);
}

#else

auto CompiledCode::operator new(const size_t sz) -> void* {
  const auto heap = GetCurrentThreadHeap();
  ASSERT(heap);
  const auto address = heap->TryAllocate(sz);
  ASSERT(address != UNALLOCATED);
  return reinterpret_cast<void*>(address);
}

void CompiledCode::operator delete(void* ptr) {
  ASSERT(ptr);
  // do nothing
}

#endif  // GEL_DISABLE_HEAP

auto CompiledCode::ToString() const -> std::string {
  ToStringHelper<CompiledCode> helper{};
  helper.AddField("start_address", GetCodeStartingAddress());
  helper.AddField("size", GetCodeSize());
  return helper;
}
}  // namespace gel
