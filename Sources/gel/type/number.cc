#include "gel/type/number.h"

#include <compare>

#include "gel/exception.h"
#include "gel/to_string_helper.h"

namespace gel {
#ifdef GEL_ENABLE_HEAP

auto Number::operator new(const size_t sz) -> void* {
  ASSERT(CurrentThreadHasHeap());
  const auto new_address = GetCurrentThreadHeap()->TryAllocate(sz);
  LOG_IF(FATAL, IsUnallocated(new_address)) << "failed to allocate memory for new nil instance.";
  return reinterpret_cast<void*>(new_address);
}

#else

auto Number::operator new(const size_t sz) -> void* {
  return reinterpret_cast<void*>(sys::malloc(sz));
}

#endif  // GEL_ENABLE_HEAP

auto Number::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= Get();
  return hash;
}

auto Number::Compare(Value* rhs) const -> std::strong_ordering {
  if (!rhs || !rhs->IsNumber())
    return std::strong_ordering::less;
  if (Get() < rhs->AsNumber()->Get())
    return std::strong_ordering::less;
  else if (Get() > rhs->AsNumber()->Get())
    return std::strong_ordering::greater;
  return std::strong_ordering::equivalent;
}

auto Number::ToString() const -> std::string {
  ToStringHelper<Number> helper{};
  helper.AddField("value", Get());
  return helper;
}

// auto Nil::GetHashCode() const -> HashCode {
//   HashCode hash{};
//   hash ^= 0xBAB3;
//   return hash;
// }
//
// auto Nil::ToString() const -> std::string {
//   ToStringHelper<Nil> helper{};
//   return helper;
// }
//
// auto Nil::Compare(Value* rhs) const -> std::strong_ordering {
//   NOT_IMPLEMENTED(ERROR);  // TODO: @s0cks implement
//   return std::strong_ordering::equivalent;
// }

}  // namespace gel
