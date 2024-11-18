#include "scheme/heap.h"

#include "scheme/os_thread.h"
#include "scheme/platform.h"
#include "scheme/section.h"

namespace scm {
Heap::Heap() :
  new_zone_(),
  old_zone_() {}

auto Heap::TryAllocate(const uword size) -> uword {
  ASSERT(size > 0);
  uword result = UNALLOCATED;
  if (size >= kLargeObjectSize) {
    if ((result = old_zone_.TryAllocate(size)) == UNALLOCATED) {
      LOG(ERROR) << "failed to allocate large object of " << units::data::byte_t(size);
      // TODO: major collection
      if ((result = old_zone_.TryAllocate(size)) == UNALLOCATED) {
        LOG(FATAL) << "failed to allocate large object of " << units::data::byte_t(size);
      }
    }
    ASSERT(result != UNALLOCATED);
    return result;
  }

  if ((result = new_zone_.TryAllocate(size)) == UNALLOCATED) {
    LOG(ERROR) << "failed to allocate new object of " << units::data::byte_t(size);
    // TODO: minor collection
    if ((result = new_zone_.TryAllocate(size)) == UNALLOCATED) {
      LOG(FATAL) << "failed to allocate new object of " << units::data::byte_t(size);
    }
  }
  ASSERT(result != UNALLOCATED);
  return result;
}

void Heap::Clear() {
  new_zone_.Clear();
  old_zone_.Clear();
}

static const ThreadLocal<Heap> heap_{};

auto Heap::GetHeap() -> Heap* {
  ASSERT(heap_);
  return heap_.Get();
}

void Heap::Init() {
  ASSERT(heap_.IsEmpty());
  heap_.Set(new Heap());
  ASSERT(heap_);
  DLOG(INFO) << "allocated: " << *(heap_.Get());
}

#ifdef SCM_DEBUG

using namespace units::data;

void PrintHeap(Heap& heap) {
  DLOG(INFO) << "Heap:";
  DLOG(INFO) << "  Total Size: " << byte_t(heap.GetTotalSize());
  PrintNewZone(heap.GetNewZone());
  PrintOldZone(heap.GetOldZone());
}

#endif  // SCM_DEBUG
}  // namespace scm