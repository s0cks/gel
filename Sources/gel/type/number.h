#ifndef GEL_NUMBER_H
#define GEL_NUMBER_H

#include <fmt/format.h>
#include <type_traits>

#include "gel/type/value.h"

namespace gel {
template <typename T>
concept is_raw_num_v = std::is_integral_v<T> || std::same_as<T, double> || std::same_as<T, float>;

using RawNumber = double;
class Number : public Value {
  friend class Number;
  friend class Double;

 private:
  RawNumber value_;

 protected:
  template <typename T>
  explicit Number(const T value)
    requires(is_raw_num_v<T>)
    :
    Value(),
    value_(static_cast<RawNumber>(value)) {}

 public:
  ~Number() override = default;

  constexpr auto Get() const -> RawNumber {
    return value_;
  }

  template <typename T>
  inline constexpr auto AsRaw() const -> T
    requires(is_raw_num_v<T>)
  {
    return static_cast<T>(Get());
  }

  DECLARE_VALUE_TYPE(Number);

 public:
  template <typename T>
  static inline auto New(const T rhs) -> Number* requires(is_raw_num_v<T>) { return new Number(rhs); }
};
}  // namespace gel

namespace fmt {
template <>
struct formatter<gel::Number> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Number& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.Get());
  }
};
}  // namespace fmt

#endif  // GEL_NUMBER_H
