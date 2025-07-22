#include "gel/type/bool.h"

#include <__compare/compare_three_way.h>
#include <compare>

#include "gel/class.h"
#include "gel/number.h"

namespace gel {
static Bool* kTrue = nullptr;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static Bool* kFalse = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void Bool::Init() {
  kTrue = NewTrue();
  kFalse = NewFalse();
}

auto Bool::Equals(Value* rhs) const -> bool {
  if (!rhs->IsBool())
    return false;
  return Get() == rhs->AsBool()->Get();
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
}  // namespace gel