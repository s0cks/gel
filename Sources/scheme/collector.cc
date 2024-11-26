#include "scheme/collector.h"

#include "scheme/common.h"
#include "scheme/heap.h"
#include "scheme/object.h"
#include "scheme/platform.h"
#include "scheme/runtime.h"

namespace scm {
Collector::Collector(Heap& heap) :
  heap_(heap) {}

auto Collector::CopyPointer(Pointer* ptr) -> Pointer* {
  ASSERT(ptr);
  const auto total_size = ptr->GetTotalSize();
  if ((next_address() + total_size) >= (heap().new_zone().fromspace() + heap().new_zone().semisize()))
    return UNALLOCATED;
  const auto next = Pointer::Copy(next_address(), ptr);
  next_address_ += total_size;
  return next;
}

auto Collector::ProcessRoot(Pointer** ptr) -> bool {
  ASSERT(ptr && (*ptr));
  const auto old_ptr = (*ptr);
  const auto value = old_ptr->GetObjectPointer();
  ASSERT(value);
  DLOG(INFO) << "processing: " << (*old_ptr) << " := " << value->ToString();
  const auto new_ptr = CopyPointer(old_ptr);
  ASSERT(new_ptr);
  new_ptr->tag().SetRememberedBit();
  DLOG(INFO) << "new_ptr: " << (*new_ptr);
  (*ptr) = new_ptr;
  return true;
}

auto Collector::ProcessRoots() -> bool {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto scope = runtime->GetCurrentScope();
  if (scope) {
    const auto scope_processed = scope->VisitLocalPointers([&](Pointer** ptr) {
      return ProcessRoot(ptr);
    });
    if (!scope_processed) {
      LOG(ERROR) << "failed to process: " << scope->ToString();
      return false;
    }
  }
  return true;
}

auto Collector::ProcessFromspace() -> bool {
  while (current_address() < heap().new_zone().fromspace()) {
    auto ptr = Pointer::At(current_address());
    ASSERT(ptr);
    curr_address_ += ptr->GetTotalSize();
  }
  return true;
}

/*
 * Cheney's Algorithm:
 * collect() =
 *  swap(fromspace, tospace)
 *  allocPtr = fromspace
 *  scanPtr = fromspace
 *  -- scan every root you've got
 *  ForEach root in the stack -- or elsewhere
 *   root = copy(root)
 *  EndForEach
 *  -- scan objects in the to-space (including objects added by this loop)
 *  While scanPtr < allocPtr
 *   ForEach reference r from o (pointed to by scanPtr)
 *    r = copy(r)
 *   EndForEach
 *   scanPtr = scanPtr + o.size() -- points to the next object in the to-space, if any
 *  EndWhile
 */
void Collector::Collect() {
  heap().new_zone().SwapSpaces();
  next_address_ = curr_address_ = heap().new_zone().fromspace();
  LOG_IF(FATAL, !ProcessRoots()) << "failed to mark roots.";
  LOG_IF(FATAL, !ProcessFromspace()) << "failed to process fromspace.";
}

void MinorCollection() {
  const auto heap = Heap::GetHeap();
  ASSERT(heap);
  Collector collector((*heap));
  collector.Collect();
}

void MajorCollection() {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}
}  // namespace scm