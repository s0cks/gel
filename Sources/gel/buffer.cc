#include "gel/buffer.h"

#include "gel/common.h"
#include "gel/heap.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/to_string_helper.h"

namespace gel {
#ifdef GEL_DISABLE_HEAP

auto Buffer::operator new(const size_t sz, const uword capacity) -> void* {
  const auto total_size = sz + (sizeof(uint8_t) * capacity);
  return malloc(total_size);
}

#else

auto Buffer::operator new(const size_t sz, const uword capacity) -> void* {
  const auto heap = Heap::GetHeap();
  ASSERT(heap);
  const auto total_size = sz + (sizeof(uint8_t) * capacity);
  const auto address = heap->TryAllocate(total_size);
  ASSERT(address != UNALLOCATED);
  return (void*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

#define DEFINE_NEW_OPERATOR(Name)                     \
  auto Name::operator new(const size_t sz) -> void* { \
    const auto heap = Heap::GetHeap();                \
    ASSERT(heap);                                     \
    const auto address = heap->TryAllocate(sz);       \
    ASSERT(address != UNALLOCATED);                   \
    return reinterpret_cast<void*>(address);          \
  }

#endif  // GEL_DISABLE_HEAP

auto Buffer::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

auto Buffer::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Buffer::ToString() const -> std::string {
  ToStringHelper<Buffer> helper;
  helper.AddField("data", data());
  return helper;
}

auto Buffer::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  const auto cls = Class::New(Object::GetClass(), "Buffer");
  ASSERT(cls);
  cls->AddFunction(proc::buffer_get_capacity::Get()->GetNative());
  cls->AddFunction(proc::buffer_get_length::Get()->GetNative());
  return cls;
}

auto Buffer::Copy(String* src) -> Buffer* {
  ASSERT(src);
  return Buffer::Copy(src->Get());
}

auto Buffer::New(const ObjectList& args) -> Buffer* {
  if (args.empty() || gel::IsNull(args[0]))
    return Buffer::New(kDefaultBufferSize);
  else if (gel::IsLong(args[0]))
    return Buffer::New(args[0]->AsLong()->Get());
  else if (gel::IsString(args[0]))
    return Buffer::Copy(args[0]->AsString());
  return Buffer::New(kDefaultBufferSize);
}

void Buffer::Init() {
  proc::buffer_get_capacity::Init();
  proc::buffer_get_length::Init();
  InitClass();
}

namespace proc {
#define BUFFER_PROCEEDURE_F(Name) NATIVE_PROCEDURE_F(buffer_##Name)

BUFFER_PROCEEDURE_F(get_length) {
  NativeArgument<0, Buffer> buffer(args);
  if (!buffer)
    return Throw(buffer.GetError());
  return ReturnNew<Long>(buffer->GetLength());
}

BUFFER_PROCEEDURE_F(get_capacity) {
  NativeArgument<0, Buffer> buffer(args);
  if (!buffer)
    return Throw(buffer.GetError());
  return ReturnNew<Long>(buffer->GetCapacity());
}

#undef BUFFER_PROCEDURE_F
}  // namespace proc
}  // namespace gel