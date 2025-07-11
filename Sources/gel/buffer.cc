#include "gel/buffer.h"

#include <cstddef>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <sstream>
#include <string>
#include <vector>

#include "gel/allocator.h"
#include "gel/common.h"
#include "gel/hashcode.h"
#include "gel/heap.h"  // IWYU pragma: keep
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/os_thread.h"  // IWYU pragma: keep
#include "gel/platform.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"

namespace gel {
DEFINE_NEW_OPERATOR(Buffer);

auto Buffer::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return kInvalidHashCode;
}

auto Buffer::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto DefaultBufferEncoding::Encode(const Buffer& rhs) const -> String* {
  return String::New(rhs.GetAsString());
}

auto DefaultBufferEncoding::Decode(const String& rhs) const -> Buffer* {
  if (rhs.IsEmpty())
    return Buffer::New(0);
  return Buffer::Copy(rhs.Get());
}

auto HexBufferEncoding::Decode(const String& rhs) const -> Buffer* {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return nullptr;
}

auto HexBufferEncoding::Encode(const Buffer& rhs) const -> String* {
  const auto buff_length = rhs.write_pos();
  const auto hex_buff_length = 1 + buff_length * 2;
  std::vector<char> hex{};
  hex.resize(hex_buff_length);
  size_t hex_length = 0;
  OPENSSL_buf2hexstr_ex(&hex[0], hex_buff_length, &hex_length, rhs.data(), rhs.write_pos(), '\0');
  std::string result(hex.data(), hex_length);
  return String::New(result);
}

auto Base64BufferEncoding::DecodeBlockData(std::string& out, const uint8_t* data, const uint64_t num_bytes) const
    -> uword {
  return EVP_DecodeBlock(reinterpret_cast<unsigned char*>(out.data()), data, static_cast<int>(num_bytes));
}

auto Base64BufferEncoding::Decode(const String& rhs) const -> Buffer* {
  std::string decoded(CalcDecodedLength(rhs) + 1, '\0');
  LOG_IF(WARNING, !DecodeBlock(decoded, rhs)) << "base64 decoding issue decoding " << rhs;
  return Buffer::Copy(decoded);
}

auto Base64BufferEncoding::EncodeBlockData(std::string& out, const uint8_t* data, const uint64_t num_bytes) const
    -> uword {
  return static_cast<uword>(
      EVP_EncodeBlock(reinterpret_cast<unsigned char*>(out.data()), data, static_cast<int>(num_bytes)));
}

auto Base64BufferEncoding::Encode(const Buffer& rhs) const -> String* {
  std::string encoded(CalcEncodedLength(rhs) + 1, '\0');
  LOG_IF(WARNING, !EncodeBlock(encoded, rhs)) << "base64 encoding issue encoding " << rhs;
  return String::New(encoded);
}

auto Buffer::ToString(String* encoding) const -> String* {
  ASSERT(encoding);
  if (HexBufferEncoding::Matches(encoding)) {
    return HexBufferEncoding{}.Encode(*this);
  } else if (Base64BufferEncoding::Matches(encoding)) {
    return Base64BufferEncoding{}.Encode(*this);
  }
default_encoding:
  ASSERT(DefaultBufferEncoding::Matches(encoding));
  return DefaultBufferEncoding{}.Encode(*this);
}

auto Buffer::ToString() const -> std::string {
  ToStringHelper<Buffer> helper;
  helper.AddField("data", (const void*)data());
  return helper;
}

auto Buffer::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Buffer");
}

auto Buffer::Copy(String* src) -> Buffer* {
  ASSERT(src);
  return Buffer::Copy(src->Get());
}

auto Buffer::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Buffer::New(const ObjectList& args) -> Buffer* {
  if (args.empty() || args[0]->IsNil())
    return Buffer::New(kDefaultBufferSize);
  else if (gel::IsLong(args[0]))
    return Buffer::New(args[0]->AsLong()->Get());
  else if (gel::IsString(args[0])) {
    RequiredNativeArgument<0, String> value(args);
    if (args.size() > 1 && gel::IsString(args[1])) {
      const auto encoding = args[1]->AsString();
      ASSERT(encoding);
      if (HexBufferEncoding::Matches(encoding)) {
        return HexBufferEncoding{}.Decode(*value);
      } else if (Base64BufferEncoding::Matches(encoding)) {
        return Base64BufferEncoding{}.Decode(*value);
      }
      ASSERT(DefaultBufferEncoding::Matches(encoding));
      return DefaultBufferEncoding{}.Decode(*value);
    }
    return Buffer::Copy(args[0]->AsString());
  }
  return Buffer::New(kDefaultBufferSize);
}

void Buffer::Init() {
  proc::buffer_get_capacity::Init();
  proc::buffer_to_string::Init();
#define REGISTER_BUFFER_PROCEDURES(Sz) \
  proc::buffer_read_uint##Sz::Init();  \
  proc::buffer_write_uint##Sz::Init();

  FOR_EACH_BUFFER_ELEMENT_SIZE(REGISTER_BUFFER_PROCEDURES);
#undef REGISTER_BUFFER_READ_PROCEDURE
  InitClass();
}

namespace proc {
#define BUFFER_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(buffer_##Name)

BUFFER_PROCEDURE_F(get_capacity) {
  REQUIRED_NATIVE_ARG(0, Buffer, buffer);
  return ReturnLong(static_cast<RawLong>(buffer->GetCapacity()));
}

BUFFER_PROCEDURE_F(to_string) {
  REQUIRED_NATIVE_ARG(0, Buffer, buffer);
  OptionalNativeArgument<1, String> encoding(args);
  if (!encoding)
    return Throw(encoding);
  if (encoding.GetValue()->IsNil())
    return Return(buffer->ToString(nullptr));
  return Return(buffer->ToString(encoding.GetValue()));
}

#define DEFINE_BUFFER_READ_PROCEDURE(Sz)                             \
  BUFFER_PROCEDURE_F(read_uint##Sz) {                                \
    NativeArgument<0, Buffer> buffer(args);                          \
    if (!buffer)                                                     \
      return Throw(buffer);                                          \
    OptionalNativeArgument<1, Long> index(args);                     \
    if (!index)                                                      \
      return Throw(index);                                           \
    const auto idx = index.HasValue() ? index.GetValue()->Get() : 0; \
    return ReturnNew<Long>(buffer->ReadUInt##Sz##At(idx));           \
  }

#define DEFINE_BUFFER_WRITE_PROCEDURE(Sz)                                 \
  BUFFER_PROCEDURE_F(write_uint##Sz) {                                    \
    REQUIRED_NATIVE_ARG(0, Buffer, buffer);                               \
    NativeArgument<1, Long> value(args);                                  \
    if (!value)                                                           \
      return Throw(value);                                                \
    OptionalNativeArgument<2, Long> index(args);                          \
    if (!index)                                                           \
      return Throw(index);                                                \
    const auto idx = index.HasValue() ? index->Get() : 0;                 \
    if (!buffer->PutUInt##Sz##At(idx, value->Get())) {                    \
      std::stringstream ss;                                               \
      ss << "failed to write uint" << Sz << " in " << buffer->ToString(); \
      return ThrowError(ss);                                              \
    }                                                                     \
    return ReturnNull();                                                  \
  }

FOR_EACH_BUFFER_ELEMENT_SIZE(DEFINE_BUFFER_READ_PROCEDURE);
FOR_EACH_BUFFER_ELEMENT_SIZE(DEFINE_BUFFER_WRITE_PROCEDURE);
#undef DEFINE_BUFFER_READ_PROCEDURE

#undef BUFFER_PROCEDURE_F
}  // namespace proc
}  // namespace gel