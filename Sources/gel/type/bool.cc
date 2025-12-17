#include "gel/type/bool.h"

#include <compare>

namespace gel {
static Bool* kTrue = nullptr;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static Bool* kFalse = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void Bool::Init() {
  kTrue = NewTrue();
  kFalse = NewFalse();
}

auto Bool::New(const bool value) -> Bool* {
  return new Bool(value);
}

auto Bool::True() -> Bool* {
  ASSERT(kTrue);
  return kTrue;
}

auto Bool::False() -> Bool* {
  ASSERT(kFalse);
  return kFalse;
}

auto Bool::Compare(Value* o) const -> std::strong_ordering {
  if (!o || !o->IsBool())
    return std::strong_ordering::less;
  const auto rhs = o->AsBool();
  return Get() <=> rhs->Get();
}

auto Bool::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= Get();
  return hash;
}

auto Bool::ToString() const -> std::string {
  return Get() ? "#T" : "#F";
}

#ifdef GEL_ENABLE_HEAP

auto Bool::operator new(const size_t sz) -> void* {
  ASSERT(CurrentThreadHasHeap());
  const auto new_address = GetCurrentThreadHeap()->TryAllocate(sz);
  LOG_IF(FATAL, IsUnallocated(new_address)) << "failed to allocate memory for new nil instance.";
  return reinterpret_cast<void*>(new_address);
}

#else

auto Bool::operator new(const size_t sz) -> void* {
  return reinterpret_cast<void*>(sys::malloc(sz));
}

#endif  // GEL_ENABLE_HEAP
}  // namespace gel
