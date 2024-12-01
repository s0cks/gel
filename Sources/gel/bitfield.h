#ifndef GEL_BITFIELD_H
#define GEL_BITFIELD_H

#include <type_traits>

#include "gel/platform.h"

namespace gel {
template <typename S, typename T, int Position, int Size = (sizeof(S) * kBitsPerByte) - Position>
class BitField {
 public:
  static constexpr auto Mask() -> S {
    return (kUWordOne << Size) - 1;
  }

  static constexpr auto MaskInPlace() -> S {
    return Mask() << Position;
  }

  static constexpr auto Decode(const S val) -> T {
    const auto u = static_cast<std::make_unsigned_t<S>>(val);
    return static_cast<T>((u >> Position) & Mask());
  }

  static constexpr auto Encode(const T val) -> S {
    const auto u = static_cast<std::make_unsigned_t<S>>(val);
    return (u & Mask()) << Position;
  }

  static constexpr auto Update(const T val, const S original) -> S {
    return Encode(val) | (~MaskInPlace() & original);
  }
};
}  // namespace gel

#endif  // GEL_BITFIELD_H
