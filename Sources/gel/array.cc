#include "gel/array.h"

#include "gel/common.h"
#include "gel/heap.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/runtime.h"

namespace gel {
ArrayBase::ArrayBase(const word init_cap) {
  if (init_cap > 0) {
    const auto new_cap = RoundUpPow2(init_cap);
    const auto address = sys::malloc(sizeof(uword) * new_cap);  // TODO: convert to gel heap allocation
    LOG_IF(FATAL, address == UNALLOCATED) << "failed to allocate GrowableArray of: " << bytes(new_cap);
    data_ = address;
    capacity_ = new_cap;
    memset((void*)data_, 0, sizeof(uword) * new_cap);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }
}

ArrayBase::~ArrayBase() {
  if (data_ != UNALLOCATED)
    sys::free((uword)data_);
}

void ArrayBase::Resize(const word new_length) {
  if (new_length > capacity_) {
    const auto new_cap = RoundUpPow2(new_length);
    const auto new_data = sys::realloc((uword)data_, sizeof(uword) * new_cap);  // TODO: convert to gel heap allocation
    LOG_IF(FATAL, new_data == UNALLOCATED) << "failed to resize GrowableArray to: " << bytes(new_cap);
    data_ = new_data;
    capacity_ = new_cap;
  }
  length_ = new_length;
}

#ifdef GEL_DISABLE_HEAP

auto ArrayBase::operator new(const size_t sz, const uword cap) -> void* {
  return sys::malloc(sz + sizeof(uword) * cap);
}

#else

auto ArrayBase::operator new(const size_t sz) -> void* {
  const auto heap = GetCurrentThreadHeap();
  ASSERT(heap);
  const auto address = heap->TryAllocate(sz);
  ASSERT(address != UNALLOCATED);
  return (void*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

#endif  // GEL_DISABLE_HEAP

Class* ArrayBase::kClass = nullptr;
void ArrayBase::InitClass() {
  ASSERT(kClass == nullptr);
  kClass = CreateClass();
  ASSERT(kClass);
}

auto ArrayBase::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

auto ArrayBase::Compare(Object* rhs) const -> int {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto ArrayBase::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Array(";
  ss << "capacity=" << GetCapacity() << ", ";
  ss << "length=" << GetLength() << ", ";
  ss << "data=";
  ss << "[";
  for (auto idx = 0; idx < GetLength(); idx++) {
    const auto value = *(GetPtrAddrAt(idx));
    ASSERT(value);
    PrintValue(ss, value->GetObjectPointer());
    if (idx < (GetLength() - 1))
      ss << ", ";
  }
  ss << "]";
  ss << ")";
  return ss.str();
}

auto ArrayBase::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsArray())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto ArrayBase::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Seq::GetClass(), "Array");
}

auto ArrayBase::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  ArrayPointerIterator iter(this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    if (!IsUnallocated(next)) {
      if (!vis->Visit((*next)))
        return false;
    }
  }
  return true;
}

auto ArrayBase::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  ArrayPointerIterator iter(this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    if (!IsUnallocated(next)) {
      if (!vis->Visit(next))
        return false;
    }
  }
  return true;
}

auto ArrayBase::VisitValues(const std::function<bool(Object*)>& vis) -> bool {
  ArrayPointerIterator iter(this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    if (!IsUnallocated(next)) {
      if (!vis((*next)->GetObjectPointer()))
        return false;
    }
  }
  return true;
}
}  // namespace gel