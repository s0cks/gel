#ifndef GEL_BUFFER_H
#define GEL_BUFFER_H

#include "gel/common.h"
#include "gel/native_procedure.h"
#include "gel/object.h"

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

class BufferEncoding {
  DEFINE_NON_COPYABLE_TYPE(BufferEncoding);

 public:
  BufferEncoding() = default;
  virtual ~BufferEncoding() = default;
  virtual auto Decode(const String* value) const -> Buffer* = 0;
  virtual auto Encode(const Buffer* value) const -> String* = 0;
};

class DefaultBufferEncoding : public BufferEncoding {
 public:
  auto Decode(const String* value) const -> Buffer* override;
  auto Encode(const Buffer* value) const -> String* override;

  static inline auto Matches(String* rhs) -> bool {
    return rhs == nullptr || (rhs && (rhs->Equals("default") || rhs->Equals("none")));
  }
};

class Base64BufferEncoding : public BufferEncoding {
 public:
  auto Decode(const String* value) const -> Buffer* override;
  auto Encode(const Buffer* value) const -> String* override;

  static inline auto Matches(String* rhs) -> bool {
    return rhs != nullptr && (rhs->Equals("b64") || rhs->Equals("base64"));
  }
};

class HexBufferEncoding : public BufferEncoding {
 public:
  auto Decode(const String* rhs) const -> Buffer* override;
  auto Encode(const Buffer* rhs) const -> String* override;

  static inline auto Matches(String* rhs) -> bool {
    return rhs != nullptr && rhs->Equals("hex");
  }
};

class Buffer : public Object {
  static constexpr const auto kDefaultBufferSize = 4096;
  static constexpr const auto kMaxBufferSize = 4 * 1024 * 1024;

 private:
  uword wpos_ = 0;
  uword rpos_ = 0;
  uword capacity_;

  Buffer(const uword capacity) :
    capacity_(capacity) {
    ASSERT(IsPow2(capacity_));
  }

  void CopyFrom(const uint8_t* src, const uword num_bytes) {
    ASSERT(src);
    ASSERT((wpos() + num_bytes) <= GetCapacity());  // TODO: resize
    memcpy(&data()[wpos()], &src[0], num_bytes);
    wpos_ += num_bytes;
  }

  template <typename T>
  inline auto ReadAt(const uint64_t pos, T* result) -> bool {
    static constexpr const auto kValueSize = sizeof(T);
    if ((pos + kValueSize) > GetCapacity()) {
      LOG(ERROR) << "cannot read " << units::data::byte_t(kValueSize) << " from " << ToString();
      return false;
    }
    rpos_ = pos + kValueSize;
    (*result) = *((T*)(data() + pos));
    return true;
  }

  template <typename T>
  inline auto Read(T* result) -> bool {
    return ReadAt<T>(rpos_, result);
  }

  template <typename T>
  inline auto PutAt(const uint64_t pos, const T value) -> bool {
    static constexpr const auto kValueSize = sizeof(T);
    if ((pos + kValueSize) > GetCapacity()) {
      LOG(ERROR) << "cannot read " << units::data::byte_t(kValueSize) << " from " << ToString();
      return false;
    }
    *((T*)(data() + pos)) = value;
    wpos_ = pos + kValueSize;
    return true;
  }

  template <typename T>
  inline auto Put(const T value) -> bool {
    return PutAt<T>(wpos(), value);
  }

 public:
  ~Buffer() override = default;

  auto GetDataAddress() const -> uword {
    return raw_ptr()->GetObjectAddress() + sizeof(Buffer);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto data() const -> uint8_t* {
    return (uint8_t*)GetDataAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto wpos() const -> uword {
    return wpos_;
  }

  auto rpos() const -> uword {
    return rpos_;
  }

  auto GetCapacity() const -> uword {
    return capacity_;
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
  static void Init();
  static auto operator new(const size_t sz, const uword capacity) -> void*;
  static inline auto New(const uword init_cap) -> Buffer* {
    ASSERT(init_cap >= 1 && init_cap <= kMaxBufferSize);
    const auto capacity = RoundUpPow2(static_cast<word>(init_cap));
    ASSERT(capacity <= kMaxBufferSize);
    return new (capacity) Buffer(capacity);
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
