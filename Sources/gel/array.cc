#include "gel/array.h"

#include "gel/common.h"
#include "gel/heap.h"
#include "gel/runtime.h"

namespace gel {
#ifdef GEL_DISABLE_HEAP

auto ArrayBase::operator new(const size_t sz, const uword cap) -> void* {
  return malloc(sz + sizeof(uword) * cap);
}

#else

auto ArrayBase::operator new(const size_t sz, const uword cap) -> void* {
  const auto heap = Heap::GetHeap();
  ASSERT(heap);
  const auto total_size = sz + sizeof(uword) * cap;
  const auto address = heap->TryAllocate(total_size);
  ASSERT(address != UNALLOCATED);
  return (void*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

#endif  // GEL_DISABLE_HEAP

Class* ArrayBase::kClass = nullptr;
void ArrayBase::InitClass() {
  ASSERT(kClass == nullptr);
  kClass = CreateClass();
  ASSERT(kClass);
}

auto ArrayBase::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

auto ArrayBase::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsArray())
    return false;
  const auto other = (ArrayBase*)rhs;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  return GetCapacity() == other->GetCapacity();
}

auto ArrayBase::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Array(";
  ss << "capacity=" << GetCapacity() << ", ";
  ss << "data=";
  ss << "[";
  for (auto idx = 0; idx < GetCapacity(); idx++) {
    const auto value = Get(idx);
    if (!value)
      continue;
    PrintValue(ss, value);
    if (idx < (GetCapacity() - 1))
      ss << ", ";
  }
  ss << "]";
  ss << ")";
  return ss.str();
}

auto ArrayBase::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Seq::GetClass(), "Array");
}

auto ArrayBase::VisitPointers(const std::function<bool(Pointer**)>& vis) -> bool {
  ArrayPointerIterator iter(this);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    if (!IsUnallocated(next)) {
      if (!vis(next))
        return false;
    }
  }
  return true;
}

void ArrayBase::Init() {
  InitClass();
  InitNative<proc::array_new>();
  InitNative<proc::array_get>();
  InitNative<proc::array_set>();
  InitNative<proc::array_length>();
}

namespace proc {
NATIVE_PROCEDURE_F(array_new) {
  ASSERT(HasRuntime());
  if (args.empty())
    return ThrowError(fmt::format("expected args to not be empty"));
  const auto length = args.size();
  const auto result = Array<Object*>::New(length);
  ASSERT(result);
  for (auto idx = 0; idx < length; idx++) {
    ASSERT(args[idx]);
    result->Set(idx, args[idx]);
  }
  return Return(result);
}

NATIVE_PROCEDURE_F(array_get) {
  ASSERT(HasRuntime());
  if (args.size() != 2)
    return ThrowError(fmt::format("expected args to be: `<array> <index>`"));
  NativeArgument<0, ArrayBase> array(args);
  NativeArgument<1, Long> index(args);
  if (index->Get() > array->GetCapacity())
    return ThrowError(fmt::format("index `{}` is out of bounds for `{}`", index->Get(), (const gel::Object&)*array));
  const auto result = array->Get(index->Get());
  return Return(result ? result : Null());
}

NATIVE_PROCEDURE_F(array_set) {
  ASSERT(HasRuntime());
  if (args.size() != 3)
    return ThrowError(fmt::format("expected args to be: `<array> <index>`"));
  if (!gel::IsArray(args[0]))
    return ThrowError(fmt::format("expected `{}` to be an Array", (*args[0])));
  const auto array = (ArrayBase*)args[0];  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  ASSERT(array);
  if (!gel::IsLong(args[1]))
    return ThrowError(fmt::format("expected `{}` to be a Long.", (*args[1])));
  const auto index = Long::Unbox(args[1]);
  if (index > array->GetCapacity())
    return ThrowError(fmt::format("index `{}` is out of bounds for `{}`", index, (const gel::Object&)*array));
  array->Set(index, args[2]);
  return DoNothing();
}

NATIVE_PROCEDURE_F(array_length) {
  ASSERT(HasRuntime());
  NativeArgument<0, ArrayBase> array(args);
  return ReturnNew<Long>(array->GetCapacity());
}
}  // namespace proc
}  // namespace gel