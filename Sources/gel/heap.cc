#include "heap.h"

#include <units.h>

#include "collector.h"
#include "common.h"
#include "platform.h"
#include "thread_local.h"
#include "zone.h"

namespace gel {
Heap::Heap() :
  new_zone_(),
  old_zone_() {}

auto Heap::TryAllocateOld(const uword size) -> uword {
  using namespace units::data;
  uword result = UNALLOCATED;
  if ((result = old_zone_.TryAllocate(size)) == UNALLOCATED) {
    LOG(ERROR) << "failed to allocate large object of " << bytes(size);
    gel::MajorCollection();
    result = old_zone_.TryAllocate(size);
    LOG_IF(FATAL, IsUnallocated(result)) << "failed to allocate large object of " << bytes(size);
  }
  ASSERT(result != UNALLOCATED);
  return result;
}

auto Heap::TryAllocateNew(const uword size) -> uword {
  using namespace units::data;
  uword result = UNALLOCATED;
  if ((result = new_zone_.TryAllocate(size)) == UNALLOCATED) {
    LOG(ERROR) << "failed to allocate new object of " << bytes(size);
    gel::MinorCollection();
    result = new_zone_.TryAllocate(size);
    LOG_IF(FATAL, IsUnallocated(result)) << "failed to allocate new object of " << bytes(size);
  }
  ASSERT(result != UNALLOCATED);
  return result;
}

auto Heap::TryAllocate(const uword size) -> uword {
  ASSERT(size > 0);
  uword result = UNALLOCATED;
  if (size >= kLargeObjectSize)
    return TryAllocateOld(size);
  return TryAllocateNew(size);
}

void Heap::Clear() {
  new_zone_.Clear();
  old_zone_.Clear();
}

static const ThreadLocal<Heap> heap_{};

auto GetCurrentThreadHeap() -> Heap* {
  return heap_.Get();
}

void Heap::Init() {
  ASSERT(heap_.IsEmpty());
  heap_.Set(new Heap());
  ASSERT(heap_);
#ifdef GEL_DEBUG
  DVLOG(100) << "heap initialized.";
  if (VLOG_IS_ON(100)) {
    PrintNewZone(GetCurrentThreadHeap()->GetNewZone());
    PrintOldZone(GetCurrentThreadHeap()->GetOldZone());
  }
#endif  // GEL_DEBUG
}

#ifdef GEL_DEBUG

using namespace units::data;

void PrintHeap(Heap& heap) {
  DLOG(INFO) << "Heap:";
  DLOG(INFO) << "  Total Size: " << bytes(heap.GetTotalSize());
  PrintNewZone(heap.GetNewZone());
  PrintOldZone(heap.GetOldZone());
}

#endif  // GEL_DEBUG
}  // namespace gel
