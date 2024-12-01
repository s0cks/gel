#include "gel/array.h"

#include "gel/common.h"
#include "gel/heap.h"

namespace gel {
#ifdef GEL_DISABLE_HEAP

auto ArrayBase::operator new(const size_t sz, const uword cap) -> void* {
  return malloc(sz + sizeof(uword) * cap);
}

#else

auto ArrayBase::operator new(const size_t sz, const uword cap) -> void* {
  const auto heap = Heap::GetHeap();
  ASSERT(heap);
  const auto total_size = sz + sizeof(uword) * cap;
  const auto address = heap->TryAllocate(total_size);
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

auto ArrayBase::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsArray())
    return false;
  const auto other = (ArrayBase*)rhs;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  return GetCapacity() == other->GetCapacity();
}

auto ArrayBase::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Array(";
  ss << "capacity=" << GetCapacity() << ", ";
  ss << "data=";
  ss << "[";
  for (auto idx = 0; idx < GetCapacity(); idx++) {
    const auto value = Get(idx);
    if (!value)
      continue;
    PrintValue(ss, value);
    if (idx < (GetCapacity() - 1))
      ss << ", ";
  }
  ss << "]";
  ss << ")";
  return ss.str();
}

auto ArrayBase::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Array");
}

auto ArrayBase::VisitPointers(const std::function<bool(Pointer**)>& vis) -> bool {
  ArrayPointerIterator iter(this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    if (!IsUnallocated(next)) {
      if (!vis(next))
        return false;
    }
  }
  return true;
}
}  // namespace gel