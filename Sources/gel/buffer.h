#ifndef GEL_BUFFER_H
#define GEL_BUFFER_H

#include "gel/common.h"
#include "gel/native_procedure.h"
#include "gel/object.h"

namespace gel {
class Buffer : public Object {
  static constexpr const auto kDefaultBufferSize = 4096;
  static constexpr const auto kMaxBufferSize = 4 * 1024 * 1024;

 private:
  uword length_;
  uword capacity_;

  Buffer(const uword length, const uword capacity) :
    length_(length),
    capacity_(capacity) {
    ASSERT(IsPow2(capacity_));
  }

  void CopyFrom(const uint8_t* src, const uword num_bytes) {
    ASSERT(src);
    ASSERT((GetLength() + num_bytes) <= GetCapacity());  // TODO: resize
    memcpy(&data()[GetLength()], &src[0], num_bytes);
    length_ += num_bytes;
  }

 public:
  ~Buffer() override = default;

  auto GetDataAddress() const -> uword {
    return raw_ptr()->GetObjectAddress() + sizeof(Buffer);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto data() const -> uint8_t* {
    return (uint8_t*)GetDataAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetLength() const -> uword {
    return length_;
  }

  auto GetCapacity() const -> uword {
    return capacity_;
  }

  DECLARE_TYPE(Buffer);

 public:
  static void Init();
  static auto operator new(const size_t sz, const uword capacity) -> void*;
  static inline auto New(const uword init_cap) -> Buffer* {
    ASSERT(init_cap >= 1 && init_cap <= kMaxBufferSize);
    const auto capacity = RoundUpPow2(static_cast<word>(init_cap));
    ASSERT(capacity <= kMaxBufferSize);
    return new (capacity) Buffer(0, capacity);
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

_DECLARE_BUFFER_PROCEDURE(get_length, "get-length");
_DECLARE_BUFFER_PROCEDURE(get_capacity, "get-capacity");

#undef _DECLARE_BUFFER_PROCEDURE
#undef DECLARE_BUFFER_PROCEDURE
}  // namespace proc
}  // namespace gel

#endif  // GEL_BUFFER_H
