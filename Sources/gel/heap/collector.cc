#include "gel/heap/collector.h"

#include <cstring>
#include <functional>

#include "gel/common.h"
#include "gel/buffer.h"
#include "gel/event_emitter.h"
#include "gel/event_loop.h"
#include "gel/heap/heap.h"
#include "gel/macro.h"
#include "gel/module.h"
#include "gel/type/object.h"
#include "gel/heap/pointer.h"
#include "gel/heap/region.h"
#include "gel/runtime.h"
#include "gel/script.h"
#include "gel/vm/stack_frame.h"
#include "gel/heap/zone.h"

namespace gel {
auto Collector::VisitRoots(const std::function<bool(Pointer**)>& vis) -> bool {
  PointerPointerVisitorWrapper wrapper = vis;
  return VisitRoots(&wrapper);
}

/**
 * Roots:
 *   - '()
 *   - All classes
 *   - _kernel Module
 *   - All StackFrames
 *   - Current thread EventLoop
 */
auto Collector::VisitRoots(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Pair::VisitEmptyPointerPointer(vis))
    return false;

  if (!Class::VisitAllClassPointerPointers(vis))
    return false;

  if (!Module::VisitAllModulePointerPointers(vis))
    return false;

  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (!runtime->VisitPointerPointers(vis))
    return false;

  CallStack::Iterator iter(runtime->GetCallStack());
  while (iter.HasNext()) {
    auto next = iter.Next();
    ASSERT(next);
    if (!next->VisitAllPointerPointers(vis))
      return false;
  }

  if (!VisitThreadEventLoopPointerPointer(vis))
    return false;
  return true;
}

Collector::Collector(Heap& heap) :
  heap_(heap) {}

auto Collector::Scavenge(Pointer* ptr) -> uword {
  ASSERT(ptr && !ptr->IsRemembered());
  const auto total_size = ptr->GetTotalSize();
  if ((next_address() + total_size) >= (heap().new_zone().fromspace() + heap().new_zone().semisize()))
    return UNALLOCATED;
  const auto new_address = next_address();
  next_address_ += total_size;
  const auto new_ptr = Pointer::Copy(new_address, ptr);
  ASSERT(new_ptr);
  new_ptr->SetRemembered();
  return new_ptr->GetStartingAddress();
}

auto Collector::Promote(Pointer* ptr) -> uword {
  ASSERT(ptr && ptr->IsRemembered());
  const auto new_ptr = heap().old_zone().TryAllocatePointer(ptr->GetObjectSize());
  ASSERT(new_ptr && new_ptr->GetObjectSize() == ptr->GetObjectSize());
  memcpy(new_ptr->GetObjectAddressPointer(), ptr->GetObjectAddressPointer(), ptr->GetObjectSize());
  return new_ptr->GetStartingAddress();
}

auto Collector::ProcessPointer(Pointer* ptr) -> uword {
  ASSERT(ptr);
  if (ptr->IsForwarding())
    return ptr->GetForwardingAddress();
  const auto new_address = ptr->IsRemembered() ? Promote(ptr) : Scavenge(ptr);
  ASSERT(new_address != UNALLOCATED);
  ptr->SetForwardingAddress(new_address);
  return new_address;
}

auto Collector::Process(Pointer** ptr) -> bool {
  const auto old_ptr = (*ptr);
  ASSERT(old_ptr);
  auto new_address = ProcessPointer(old_ptr);
  LOG_IF(FATAL, new_address == UNALLOCATED) << "failed to forward: " << *(old_ptr);
  (*ptr) = Pointer::At(new_address);
  DLOG(INFO) << "forwarded: " << *(old_ptr) << " => " << *(*ptr) << "  ;;  " << (*ptr)->GetObjectPointer()->ToString();
  return true;
}

auto Collector::ProcessRoots() -> bool {
  return VisitRoots(this);
}

auto Collector::Visit(Pointer** ptr) -> bool {
  ASSERT(ptr && (*ptr));
  return Process(ptr);
}

auto Collector::ProcessFromspace() -> bool {
  DVLOG(100) << "processing fromspace....";
  while (current_address() < next_address_) {
    auto ptr = Pointer::At(current_address());
    ASSERT(ptr);
    DLOG(INFO) << "processing: " << (*ptr) << " ;; " << ptr->GetObjectPointer()->ToString();
    if (!ptr->VisitPointerPointers(this))
      return false;
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
  LOG_IF(FATAL, !ProcessRoots()) << "failed to process roots.";
  LOG_IF(FATAL, !ProcessFromspace()) << "failed to process fromspace.";
  heap().new_zone().SetCurrent(next_address_);
  memset(heap().new_zone().GetTospacePointer(), 0, heap().new_zone().semisize());
}

#ifdef GEL_DEBUG
static auto PrintRoot(Pointer** ptr) -> bool {
  ASSERT(ptr && (*ptr)->GetObjectPointer());
  LOG(INFO) << "- " << (*ptr)->GetObjectPointer()->ToString() << " ;; " << *(*ptr);
  return true;
}

void PrintRoots() {
  LOG(INFO) << "roots:";
  LOG_IF(FATAL, !Collector::VisitRoots(&PrintRoot)) << "failed to print roots.";
}
#endif  // GEL_DEBUG

void MinorCollection() {
  const auto heap = GetCurrentThreadHeap();
  ASSERT(heap);

#ifdef GEL_DEBUG
  LOG(INFO) << "NewZone before:";
  PrintNewZone(heap->GetNewZone());
  PrintRoots();
#endif  // GEL_DEBUG

  Collector collector((*heap));
  collector.Collect();

#ifdef GEL_DEBUG
  LOG(INFO) << "NewZone after:";
  PrintNewZone(heap->GetNewZone());
  PrintRoots();
#endif  // GEL_DEBUG
}

void MajorCollection() {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}
}  // namespace gel
