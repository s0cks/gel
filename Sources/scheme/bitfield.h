#ifndef SCM_BITFIELD_H
#define SCM_BITFIELD_H

#include <type_traits>

#include "scheme/platform.h"

namespace scm {
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
}  // namespace scm

#endif  // SCM_BITFIELD_H
