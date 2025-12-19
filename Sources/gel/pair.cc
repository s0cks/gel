#include "pair.h"

#include "number.h"
#include "to_string_helper.h"

namespace gel {
auto Pair::New(const ObjectList& args) -> Pair* {
  if (args.empty())
    return Pair::Empty();
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Pair::Compare(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsPair())
    return false;
  if (GetFirst() < rhs->AsPair()->GetFirst())
    return true;
  return GetSecond() < rhs->AsPair()->GetSecond();
}

Field* Pair::kFirstField = nullptr;
Field* Pair::kSecondField = nullptr;
auto Pair::CreateClass() -> Class* {
  const auto cls = Class::New(Seq::GetClass(), kClassName);
  ASSERT(cls);
  kFirstField = cls->AddField("first");
  ASSERT(kFirstField);
  kSecondField = cls->AddField("second");
  ASSERT(kSecondField);
  return cls;
}

auto Pair::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Visit(GetFirst(), *vis))
    return false;
  if (!Visit(GetSecond(), *vis))
    return false;
  return true;
}

auto Pair::Equals(Object* rhs) const -> bool {
  if (!rhs->IsPair())
    return false;
  const auto other = rhs->AsPair();
  return GetFirst()->Equals(other->GetFirst()) && GetSecond()->Equals(other->GetSecond());
}

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

auto Pair::VisitEmptyPointerPointer(const std::function<bool(Pointer**)>& vis) -> bool {
  ASSERT(vis);
  ASSERT(kEmptyPair);
  auto empty = kEmptyPair->raw_ptr();
  if (!vis(&empty))
    return false;
  if (!kEmptyPair->raw_ptr()->Equals(empty))
    kEmptyPair = empty->As<Pair>();
  return true;
}

auto Pair::VisitEmptyPointerPointer(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  ASSERT(kEmptyPair);
  auto empty = kEmptyPair->raw_ptr();
  if (!vis->Visit(&empty))
    return false;
  if (!kEmptyPair->raw_ptr()->Equals(empty))
    kEmptyPair = empty->As<Pair>();
  return true;
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
    result = Pair::New(Long::New(static_cast<RawLong>(idx)), result);
    if (idx == 0)
      break;
  }
  return result;
}
}  // namespace gel
