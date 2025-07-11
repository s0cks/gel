#include "gel/nil.h"

#include "gel/allocator.h"
#include "gel/common.h"
#include "gel/heap.h"
#include "gel/os_thread.h"
#include "gel/thread_local.h"
#include "gel/to_string_helper.h"

namespace gel {
static LazyThreadLocal<Nil> instance_([]() -> Nil* {
  return Nil::New();
});

DEFINE_NEW_OPERATOR(Nil);

auto Nil::Compare(Object* rhs) const -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: @s0cks implement
  return false;
}

auto Nil::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Nil::New(const ObjectList& args) -> Nil* {
  ASSERT(args.empty());
  return New();
}

auto Nil::Get() -> Nil* {
  return instance_;
}

auto Nil::ToString() const -> std::string {
  ToStringHelper<Nil> helper{};
  return helper;
}

auto Nil::Equals(Object* rhs) const -> bool {
  return rhs && rhs->IsNil();
}

auto Nil::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= 0xBAB3;
  return hash;
}
}  // namespace gel