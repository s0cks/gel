#include "gel/vec.h"

#include <cstddef>
#include <string>
#include <vector>

#include "gel/common.h"
#include "gel/heap.h"
#include "gel/object.h"
#include "gel/platform.h"
#include "gel/to_string_helper.h"

namespace gel {
#ifdef GEL_DISABLE_HEAP

auto Vec2::operator new(const size_t sz, const uword cap) -> void* {
  return sys::malloc(sz + sizeof(uword) * cap);
}

auto Vec3::operator new(const size_t sz, const uword cap) -> void* {
  return sys::malloc(sz + sizeof(uword) * cap);
}

#else

auto Vec2::operator new(const size_t sz) -> void* {
  const auto heap = GetCurrentThreadHeap();
  ASSERT(heap);
  const auto address = heap->TryAllocate(sz);
  ASSERT(address != UNALLOCATED);
  return (void*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

auto Vec3::operator new(const size_t sz) -> void* {
  const auto heap = GetCurrentThreadHeap();
  ASSERT(heap);
  const auto address = heap->TryAllocate(sz);
  ASSERT(address != UNALLOCATED);
  return (void*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

#endif  // GEL_DISABLE_HEAP

auto Vec2::ToString() const -> std::string {
  ToStringHelper<Vec2> helper{};
  helper.AddField("x", GetX());
  helper.AddField("y", GetY());
  return helper;
}

auto Vec2::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsVec2())
    return false;
  const auto vec2 = rhs->AsVec2();
  return GetX() == vec2->GetX() && GetY() == vec2->GetY();
}

auto Vec2::GetHashCode() const -> HashCode {
  uword hash = 0;
  CombineHash(hash, GetX());
  CombineHash(hash, GetY());
  return hash;
}

auto Vec2::CreateClass() -> Class* {
  return Class::New(Seq::GetClass(), "vec2");
}

static inline auto IsNumberPair(Object* rhs) -> bool {
  if (!rhs || !rhs->IsPair())
    return false;
  const auto pair = rhs->AsPair();
  ASSERT(pair);
  return gel::IsNumber(pair->GetFirst()) && gel::IsNumber(pair->GetSecond());
}

auto Vec2::New(Pair* rhs) -> Vec2* {
  if (!IsNumberPair(rhs))
    return nullptr;
  return New(rhs->GetFirst()->AsNumber(), rhs->GetSecond()->AsNumber());
}

auto Vec2::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Vec2::New(const std::vector<Object*>& args) -> Vec2* {
  if (args.empty())
    return New(glm::vec2(0));
  else if (args[0]->IsVec2())
    return New(args[0]->AsVec2()->Get());
  else if (IsNumberPair(args[0]))
    return New(args[0]->AsPair());
  else if (args.size() == 2 && gel::IsNumber(args[0]) && gel::IsNumber(args[1]))
    return New(args[0]->AsNumber(), args[1]->AsNumber());
  else if (args.size() == 1 && gel::IsNumber(args[0]))
    return New(args[0]->AsNumber(), args[0]->AsNumber());
  return nullptr;
}

auto Vec3::ToString() const -> std::string {
  ToStringHelper<Vec3> helper{};
  helper.AddField("x", GetX());
  helper.AddField("y", GetY());
  helper.AddField("z", GetZ());
  return helper;
}

auto Vec3::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Vec3::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsVec2())
    return false;
  const auto vec3 = rhs->AsVec3();
  return GetX() == vec3->GetX() && GetY() == vec3->GetY() && GetZ() == vec3->GetZ();
}

auto Vec3::GetHashCode() const -> HashCode {
  uword hash = 0;
  CombineHash(hash, GetX());
  CombineHash(hash, GetY());
  CombineHash(hash, GetZ());
  return hash;
}

auto Vec3::CreateClass() -> Class* {
  return Class::New(Seq::GetClass(), "vec3");
}

auto Vec3::New(const std::vector<Object*>& args) -> Vec3* {
  if (args.empty())
    return New(glm::vec3(0));
  else if (args[0]->IsVec3())
    return New(args[0]->AsVec3()->Get());
  else if (args.size() == 3 && gel::IsNumber(args[0]) && gel::IsNumber(args[1]) && gel::IsNumber(args[2]))
    return New(args[0]->AsNumber(), args[1]->AsNumber(), args[2]->AsNumber());
  else if (args.size() == 1 && gel::IsNumber(args[0]))
    return New(args[0]->AsNumber(), args[0]->AsNumber(), args[0]->AsNumber());
  NOT_IMPLEMENTED(ERROR);  // TODO: implement?
  return nullptr;
}
}  // namespace gel