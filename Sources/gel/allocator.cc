#include "allocator.h"

#include "common.h"
#include "platform.h"
#include "pointer.h"

namespace gel {
auto Allocator::malloc(const uword size) -> void* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return nullptr;
}

auto Allocator::realloc(const void* ptr, const uword size) -> void* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return nullptr;
}

void Allocator::free(const void* ptr) {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto HeapObject::raw_ptr() const -> Pointer* {
  const auto address = GetStartingAddress() - sizeof(Pointer);
  ASSERT(address >= UNALLOCATED);
  return Pointer::At(address);
}

namespace sys {
auto Allocator::malloc(const uword size) -> void* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Allocator::realloc(const void* ptr, const uword size) -> void* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return nullptr;
}

void Allocator::free(const void* ptr) {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}
}  // namespace sys
}  // namespace gel
