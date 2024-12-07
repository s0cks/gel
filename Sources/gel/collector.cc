#include "gel/collector.h"

#include "gel/common.h"
#include "gel/heap.h"
#include "gel/module.h"
#include "gel/object.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/runtime.h"
#include "gel/zone.h"

namespace gel {
auto VisitRoots(PointerPointerVisitor* vis) -> bool {
  return VisitRoots([vis](Pointer** ptr) {
    return vis->Visit(ptr);
  });
}

auto VisitRoots(const std::function<bool(Pointer**)>& vis) -> bool {
  if (!HasRuntime())
    return false;
  if (!Class::VisitClassPointers(vis)) {
    LOG(ERROR) << "failed to visit Class pointers.";
    return false;
  }
  if (!Module::VisitModulePointers(vis)) {
    LOG(ERROR) << "failed to visit Module pointers.";
    return false;
  }
  // TODO: should we visit the current local scope?
  return true;
}

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

auto Collector::Process(Pointer** ptr) -> bool {
  ASSERT(ptr && (*ptr));
  const auto old_ptr = (*ptr);
  const auto value = old_ptr->GetObjectPointer();
  ASSERT(value);
  DLOG(INFO) << "processing: " << (*old_ptr) << " := " << value->ToString();
  const auto new_ptr = CopyPointer(old_ptr);
  ASSERT(new_ptr);
  new_ptr->tag().SetRememberedBit();
  old_ptr->SetForwardingAddress(new_ptr->GetStartingAddress());
  return true;
}

void Collector::ProcessRoots() {
  const auto vis = [this](Pointer** ptr) {
    return Process(ptr);
  };
  DLOG(INFO) << "processing roots....";
  LOG_IF(FATAL, !VisitRoots(vis)) << "failed to visit roots.";
}

class PointerNotifier : public PointerPointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(PointerNotifier);

 public:
  PointerNotifier() = default;
  ~PointerNotifier() override = default;

  auto Visit(Pointer** ptr) -> bool override {
    ASSERT(ptr && (*ptr)->IsForwarding());
    const auto old_ptr = (*ptr);
    const auto new_ptr = (*ptr) = Pointer::At((*ptr)->GetForwardingAddress());
    VLOG(1) << "forwarded " << (*old_ptr) << " => " << (*new_ptr);
    return true;
  }
};

void Collector::NotifyRoots() {
  PointerNotifier notifier;
  LOG_IF(FATAL, !VisitRoots(&notifier)) << "failed to notify roots.";
}

auto Collector::Visit(Pointer** ptr) -> bool {
  ASSERT(ptr && (*ptr));
  if ((*ptr)->IsForwarding())
    return true;
  return Process(ptr);
}

auto Collector::ProcessFromspace() -> bool {
  DLOG(INFO) << "processing fromspace....";
  while (current_address() < next_address_) {
    auto ptr = Pointer::At(current_address());
    ASSERT(ptr);
    DLOG(INFO) << "processing: " << (*ptr) << " ;; " << ptr->GetObjectPointer()->ToString();
    if (!ptr->VisitPointers(this))
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
  ProcessRoots();
  LOG_IF(FATAL, !ProcessFromspace()) << "failed to process fromspace.";
  NotifyRoots();
  heap().new_zone().SetCurrent(next_address_);
}

void MinorCollection() {
  const auto heap = Heap::GetHeap();
  ASSERT(heap);

  static const auto kPrintRoot = [](Pointer** ptr) {
    ASSERT(ptr && (*ptr)->GetObjectPointer());
    LOG(INFO) << "- " << (void*)(*ptr) << " ;; " << *(*ptr) << " " << (*ptr)->GetObjectPointer()->ToString();
    return true;
  };

  LOG(INFO) << "NewZone before:";
  PrintNewZone(heap->GetNewZone());
  LOG(INFO) << "roots:";
  LOG_IF(FATAL, !VisitRoots(kPrintRoot)) << "failed to visit roots.";
  Collector collector((*heap));
  collector.Collect();

  LOG(INFO) << "NewZone after:";
  PrintNewZone(heap->GetNewZone());
  LOG(INFO) << "roots:";
  LOG_IF(FATAL, !VisitRoots(kPrintRoot)) << "failed to visit roots.";
}

void MajorCollection() {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}
}  // namespace gel