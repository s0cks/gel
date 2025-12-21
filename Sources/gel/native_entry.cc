#include "native_entry.h"

#include "runtime.h"

namespace gel {
auto NativeFnEntry::Return(Object* rhs) const -> bool {
  ASSERT(rhs);
  GetRuntime()->GetCallStack()->SetReturnAddress(rhs->GetStartingAddress());
  return DoNothing();
}

auto NativeFn::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto NativeFnEntry::ThrowNotImplementedError() const -> bool {
  std::stringstream ss;
  if (HasNative()) {
    ss << "NativeFn `" << GetNative()->GetSymbol()->GetFullyQualifiedName() << "` is ";
  }
  ss << "not implemented!";
  return ThrowError(ss);
}
}  // namespace gel
