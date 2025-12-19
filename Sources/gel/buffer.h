#ifndef GEL_BUFFER_H
#define GEL_BUFFER_H

#include <cstdint>
#include <cstring>
#include <string>
#include <units.h>

#include "common.h"
#include "native_procedure.h"
#include "object.h"
#include "platform.h"

namespace gel {
#define FOR_EACH_BUFFER_ELEMENT_SIZE(V) \
  V(8)                                  \
  V(16)                                 \
  V(32)                                 \
  V(64)

#define FOR_EACH_BUFFER_ENCODING(V) \
  V(Default)                        \
  V(Hex)                            \
  V(Base64)

class Buffer : public Object {
  static constexpr const auto kDefaultBufferSize = 4096;
  static constexpr const auto kMaxBufferSize = 4 * 1024 * 1024;

 private:
  uword write_pos_ = 0;
  uword read_pos_ = 0;
  uword capacity_;

  Buffer(const uword capacity) :
    capacity_(capacity) {
    ASSERT(IsPow2(capacity_));
  }

  void CopyFrom(const uint8_t* src, const uword num_bytes) {
    ASSERT(src);
    ASSERT((write_pos() + num_bytes) <= GetCapacity());  // TODO: resize
    memcpy(&data()[write_pos()], &src[0], num_bytes);
    write_pos_ += num_bytes;
  }

  template <typename T>
  inline auto ReadAt(const uint64_t pos, T* result) -> bool {
    static constexpr const auto kValueSize = sizeof(T);
    if ((pos + kValueSize) > GetCapacity()) {
      LOG(ERROR) << "cannot read " << units::data::bytes(kValueSize) << " from " << ToString();
      return false;
    }
    read_pos_ = pos + kValueSize;
    (*result) = *((T*)(data() + pos));
    return true;
  }

  template <typename T>
  inline auto Read(T* result) -> bool {
    return ReadAt<T>(read_pos_, result);
  }

  template <typename T>
  inline auto PutAt(const uint64_t pos, const T value) -> bool {
    static constexpr const auto kValueSize = sizeof(T);
    if ((pos + kValueSize) > GetCapacity()) {
      LOG(ERROR) << "cannot read " << units::data::bytes(kValueSize) << " from " << ToString();
      return false;
    }
    *((T*)(data() + pos)) = value;
    write_pos_ = pos + kValueSize;
    return true;
  }

  template <typename T>
  inline auto Put(const T value) -> bool {
    return PutAt<T>(write_pos(), value);
  }

 public:
  ~Buffer() override = default;

  auto GetDataAddress() const -> uword {
    return raw_ptr()->GetObjectAddress() + sizeof(Buffer);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto data() const -> uint8_t* {
    return (uint8_t*)GetDataAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  constexpr auto write_pos() const -> uword {
    return write_pos_;
  }

  auto rpos() const -> uword {
    return read_pos_;
  }

  auto GetCapacity() const -> uword {
    return capacity_;
  }

  inline auto GetAsString() const -> std::string {
    return {(const char*)data(), write_pos()};  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

#define DEFINE_READ_SIZE(Sz)                                                                                       \
  auto ReadUInt##Sz##At(const uint64_t pos)->uint##Sz##_t {                                                        \
    uint##Sz##_t result = 0;                                                                                       \
    LOG_IF(ERROR, !ReadAt<uint##Sz##_t>(pos, &result)) << "failed to read uint" << Sz << "_t from " << ToString(); \
    return result;                                                                                                 \
  }

#define DEFINE_WRITE_SIZE(Sz)                                              \
  auto PutUInt##Sz##At(const uint64_t pos, const uint##Sz##_t val)->bool { \
    return PutAt<uint##Sz##_t>(pos, val);                                  \
  }

  FOR_EACH_BUFFER_ELEMENT_SIZE(DEFINE_READ_SIZE);
  FOR_EACH_BUFFER_ELEMENT_SIZE(DEFINE_WRITE_SIZE);
#undef DEFINE_READ_SIZE

  auto ToString(String* encoding) const -> String*;
  DECLARE_TYPE(Buffer);

 public:
  friend auto operator<<(std::ostream& stream, const Buffer& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

 public:
  static void Init();

  static inline auto New(const uword init_cap) -> Buffer* {
    ASSERT(init_cap >= 1 && init_cap <= kMaxBufferSize);
    const auto capacity = RoundUpPow2(static_cast<word>(init_cap));
    ASSERT(capacity <= kMaxBufferSize);
    return new Buffer(capacity);
  }

  static inline auto Copy(const uint8_t* data, const uword num_bytes) -> Buffer* {
    ASSERT(data);
    ASSERT(num_bytes >= 1);
    const auto buffer = Buffer::New(num_bytes);
    ASSERT(buffer);
    buffer->CopyFrom(data, num_bytes);
    return buffer;
  }

  static inline auto Copy(const std::string& src) -> Buffer* {
    if (src.empty())
      return New(kDefaultBufferSize);
    return Copy((const uint8_t*)src.data(), src.length());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  static auto Copy(String* src) -> Buffer*;
};

class BufferEncoding {
  DEFINE_NON_COPYABLE_TYPE(BufferEncoding);

 public:
  BufferEncoding() = default;
  virtual ~BufferEncoding() = default;
  virtual auto Decode(const String& value) const -> Buffer* = 0;
  virtual auto Encode(const Buffer& value) const -> String* = 0;
};

class DefaultBufferEncoding : public BufferEncoding {
 public:
  auto Decode(const String& value) const -> Buffer* override;
  auto Encode(const Buffer& value) const -> String* override;

  static inline auto Matches(String* rhs) -> bool {
    return rhs == nullptr || (rhs && (rhs->Equals("default") || rhs->Equals("none")));
  }
};

class Base64BufferEncoding : public BufferEncoding {
 private:
  auto EncodeBlockData(std::string& out, const uint8_t* data, const uint64_t num_bytes) const -> uword;

  inline auto EncodeBlock(std::string& encoded, const Buffer& buff) const -> bool {
    return EncodeBlockData(encoded, buff.data(), buff.write_pos()) == (encoded.capacity() - 2);
  }

  auto DecodeBlockData(std::string& decoded, const uint8_t* data, const uint64_t num_bytes) const -> uword;

  inline auto DecodeBlockData(std::string& decoded, const std::string& in) const -> uword {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    return DecodeBlockData(decoded, (const uint8_t*)in.data(), in.size());
  }

  inline auto DecodeBlock(std::string& decoded, const String& data) const -> bool {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    return DecodeBlockData(decoded, data.Get()) == (decoded.capacity() - 2);
  }

 public:
  auto Decode(const String& value) const -> Buffer* override;
  auto Encode(const Buffer& value) const -> String* override;

  static inline auto Matches(String* rhs) -> bool {
    return rhs != nullptr && (rhs->Equals("b64") || rhs->Equals("base64"));
  }

  static inline constexpr auto CalcEncodedLength(const Buffer& rhs) -> uword {
    return 4 * ((rhs.write_pos() + 2) / 3);
  }

  static inline constexpr auto CalcDecodedLength(const String& rhs) -> uword {
    return 3 * rhs.GetLength() / 4;
  }
};

class HexBufferEncoding : public BufferEncoding {
 public:
  auto Decode(const String& rhs) const -> Buffer* override;
  auto Encode(const Buffer& rhs) const -> String* override;

  static inline auto Matches(String* rhs) -> bool {
    return rhs != nullptr && rhs->Equals("hex");
  }
};

namespace proc {
#define _DECLARE_BUFFER_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(buffer_##Name, "Buffer:" Sym)
#define DECLARE_BUFFER_PROCEDURE(Name)       _DECLARE_BUFFER_PROCEDURE(Name, #Name);

_DECLARE_BUFFER_PROCEDURE(get_capacity, "get-capacity");
_DECLARE_BUFFER_PROCEDURE(to_string, "to-string");

#define DECLARE_BUFFER_PROCEDURES(Sz)                        \
  _DECLARE_BUFFER_PROCEDURE(read_uint##Sz, "read-uint" #Sz); \
  _DECLARE_BUFFER_PROCEDURE(write_uint##Sz, "write-uint" #Sz);

FOR_EACH_BUFFER_ELEMENT_SIZE(DECLARE_BUFFER_PROCEDURES);
#undef DECLARE_BUFFER_PROCEDURES

#undef _DECLARE_BUFFER_PROCEDURE
#undef DECLARE_BUFFER_PROCEDURE
}  // namespace proc
}  // namespace gel

#endif  // GEL_BUFFER_H
