#include "gel/assembler_base.h"

#include "gel/common.h"
#include "gel/memory_region.h"
#include "gel/platform.h"

namespace gel {
AssemblerBuffer::AssemblerBuffer(const uword init_size) {
  if (init_size <= 0) {
    DLOG(WARNING) << "cannot allocate AssemblerBuffer of size: " << init_size;
    return;
  }
  MemoryRegion region(init_size, MemoryRegion::kReadWrite);
  start_ = current_ = region.GetStartingAddress();
  asize_ = region.GetSize();
}

AssemblerBuffer::~AssemblerBuffer() {
  if (start_ == UNALLOCATED || asize_ == UNALLOCATED)
    return;
  MemoryRegion region(start_, asize_);
  region.FreeRegion();
}
}  // namespace gel