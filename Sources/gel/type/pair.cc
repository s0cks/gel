#include "gel/type/pair.h"

#include "gel/to_string_helper.h"
#include "gel/type/number.h"

namespace gel {
auto Pair::Compare(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsPair())
    return false;
  if (GetFirst() < rhs->AsPair()->GetFirst())
    return true;
  return GetSecond() < rhs->AsPair()->GetSecond();
}

// auto Pair::VisitPointers(PointerVisitor* vis) -> bool {
//   ASSERT(vis);
//   if (!Visit(GetFirst(), *vis))
//     return false;
//   if (!Visit(GetSecond(), *vis))
//     return false;
//   return true;
// }

auto Pair::ToString() const -> std::string {
  ToStringHelper<Pair> helper;
  if (HasFirst())
    helper.AddField("first", GetFirst());
  if (HasSecond())
    helper.AddField("second", GetSecond());
  return helper;
}

static Pair* kEmptyPair = nullptr;
auto Pair::Empty() -> Pair* {
  if (kEmptyPair)
    return kEmptyPair;
  return kEmptyPair = Pair::NewEmpty();
}

auto Pair::GetHashCode() const -> HashCode {
  HashCode hash{};
  if (HasFirst())
    hash ^= *(GetFirst());
  if (HasSecond())
    hash ^= *(GetSecond());
  return hash;
}

auto ListFromRange(const uint64_t from, const uint64_t to) -> Object* {
  auto first = std::min(from, to);
  auto last = std::max(from, to);
  Object* result = Nil::Get();
  for (auto idx = last; idx >= first; idx--) {
    result = Pair::New(Number::New(static_cast<RawNumber>(idx)), result);
    if (idx == 0)
      break;
  }
  return result;
}
}  // namespace gel
