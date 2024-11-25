#include "scheme/collector.h"

#include "scheme/common.h"
#include "scheme/heap.h"
#include "scheme/object.h"
#include "scheme/platform.h"
#include "scheme/runtime.h"

namespace scm {
Collector::Collector(Heap& heap) :
  heap_(heap) {}

auto Collector::CopyObject(Pointer* ptr) -> Pointer* {
  ASSERT(ptr);
  const auto total_size = ptr->GetTotalSize();
  if ((next_address() + total_size) >= heap().new_zone().semisize())
    return UNALLOCATED;
  const auto next = Pointer::Copy(next_address(), ptr);
  next_address_ += total_size;
  return next;
}

auto Collector::ProcessRoots() -> bool {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto scope = runtime->GetCurrentScope();
  ASSERT(scope);
  if (!scope->Accept(this))
    return false;
  return false;
}

auto Collector::ProcessReferences(Pointer* ptr) -> bool {
  ASSERT(ptr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
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
auto Collector::Visit(Pointer* ptr) -> bool {
  ASSERT(ptr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

void Collector::Collect() {
  heap().new_zone().SwapSpaces();
  LOG_IF(FATAL, !ProcessRoots()) << "failed to mark roots.";
  curr_address_ = heap().new_zone().fromspace();
  while (current_address() < heap().new_zone().fromspace()) {
    auto ptr = Pointer::At(current_address());
    ASSERT(ptr);
    LOG_IF(FATAL, !ProcessReferences(ptr)) << "failed to process references for: " << (*ptr);
    curr_address_ += ptr->GetTotalSize();
  }
}

void MinorCollection() {
  const auto heap = Heap::GetHeap();
  ASSERT(heap);
  Collector collector((*heap));
  collector.Collect();
}
}  // namespace scm