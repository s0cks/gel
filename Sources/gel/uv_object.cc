#include "gel/common.h"
#include "gel/object.h"
#include "gel/to_string_helper.h"

namespace gel {
auto Loop::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Loop");
}

auto Loop::ToString() const -> std::string {
  ToStringHelper<Loop> helper;
  helper.AddField("loop", (const void*)Get());
  return helper;
}

auto Loop::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsLoop())
    return false;
  const auto other = rhs->AsLoop();
  ASSERT(other);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Loop::New(const ObjectList& rhs) -> Loop* {
  return New();
}

void Loop::Run(const Loop::RunMode mode) {
  uv_run(Get(), mode);  // TODO: should return this value
}
}  // namespace gel