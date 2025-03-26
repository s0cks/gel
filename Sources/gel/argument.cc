#include "gel/argument.h"

#include "gel/common.h"
#include "gel/heap.h"
#include "gel/to_string_helper.h"

namespace gel {
Class* Argument::kClass = nullptr;
auto Argument::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Argument");
}

void Argument::InitClass() {
  ASSERT(kClass == nullptr);
  kClass = CreateClass();
  ASSERT(kClass);
}

#ifdef GEL_DISABLE_HEAP

auto Array::operator new(const size_t sz) -> void* {
  return malloc(sz);
}

#else

auto Argument::operator new(const size_t sz) -> void* {
  const auto heap = GetCurrentThreadHeap();
  ASSERT(heap);
  const auto address = heap->TryAllocate(sz);
  ASSERT(address != UNALLOCATED);
  return reinterpret_cast<void*>(address);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

#endif  // GEL_DISABLE_HEAP

auto Argument::HashCode() const -> uword {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return 0;
}

auto Argument::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!VisitPointerPointer(vis, &name_))
    return false;
  return true;
}

auto Argument::ToString() const -> std::string {
  ToStringHelper<Argument> helper;
  helper.AddField("index", GetIndex());
  helper.AddField("name", GetName());
  helper.AddField("optional", IsOptional());
  helper.AddField("vararg", IsVararg());
  return helper;
}

auto Argument::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODOD: implement
  return -1;
}

auto Argument::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsArgument())
    return false;
  const auto arg = ((Argument*)rhs);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  ASSERT(arg);
  return GetIndex() == arg->GetIndex() && GetName() == arg->GetName() && (IsVararg() == arg->IsVararg()) &&
         (IsOptional() == arg->IsOptional());
}
}  // namespace gel