#ifndef GEL_HASHCODE_H
#define GEL_HASHCODE_H

#include <concepts>
#include <ios>

#include "gel/common.h"
#include "gel/platform.h"

namespace gel {
using RawHashCode = uword;

static constexpr const RawHashCode kInvalidHashCode = 0x0;

template <class T>
concept HasGetHashCode = requires(T value) {
  { value.GetHashCode() } -> std::convertible_to<RawHashCode>;
};

class HashCode {
  DEFINE_DEFAULT_COPYABLE_TYPE(HashCode);

 private:
  RawHashCode value_;

 public:
  constexpr HashCode(RawHashCode value = kInvalidHashCode) :
    value_(value) {}
  ~HashCode() = default;

  inline constexpr auto value() const -> RawHashCode {
    return value_;
  }

  inline constexpr auto IsValid() const -> bool {
    return value() != kInvalidHashCode;
  }

  friend auto operator<<(std::ostream& stream, const HashCode& rhs) -> std::ostream& {
    ostream_guard guard(stream);
    return stream << "0x" << std::hex << std::nouppercase << rhs.value();
  }

  inline constexpr auto operator==(const HashCode& rhs) const -> bool {
    return value() == rhs.value();
  }

  inline constexpr auto operator==(const RawHashCode& rhs) const -> bool {
    return value() == rhs;
  }

  inline constexpr auto operator!=(const HashCode& rhs) const -> bool {
    return value() != rhs.value();
  }

  inline constexpr auto operator!=(const RawHashCode& rhs) const -> bool {
    return value() != rhs;
  }

  inline constexpr auto operator<(const HashCode& rhs) const -> bool {
    return value() < rhs.value();
  }

  inline constexpr auto operator<(const RawHashCode& rhs) const -> bool {
    return value() < rhs;
  }

  inline auto operator^=(const RawHashCode& rhs) -> HashCode& {
    value_ ^= rhs + 0x9e3779b9 + (value_ << 6) + (value_ >> 2);  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
    return *this;
  }

  template <typename T>
  inline auto operator^=(const T& rhs) -> HashCode& {
    std::hash<T> h{};
    return operator^=(static_cast<RawHashCode>(h(rhs)));
  }

  template <HasGetHashCode T>
  inline auto operator^=(const T& rhs) -> HashCode& {
    return operator^=((RawHashCode)rhs.GetHashCode());
  }

  inline constexpr operator RawHashCode() const {
    return value();
  }

  auto operator=(const RawHashCode& rhs) -> HashCode& {
    value_ = rhs;
    return *this;
  }
};
}  // namespace gel

#endif  // GEL_HASHCODE_H
