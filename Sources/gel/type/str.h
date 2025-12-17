#ifndef GEL_STR_H
#define GEL_STR_H

#include <string>
#include <fmt/format.h>

#include "gel/type/value.h"

namespace gel {
class Str : public Value {
private:
  std::string value_{};
 protected:
  Str() = default;
  explicit Str(std::string value) :
    Value(),
    value_(std::move(value)) {}

 public:
  ~Str() override = default;

  auto str() const -> const std::string& {
    return value_;
  }

  auto c_str() const -> const char* {
    return str().c_str();
  }

  operator std::string() const {
    return str();
  }

  DECLARE_VALUE_TYPE(Str);

 public:
  inline friend auto operator<<(std::ostream& stream, const Str& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

 public:
  static auto Empty() -> Str*;
  static auto New() -> Str*;
  static inline auto New(const std::string& value) -> Str* {
    return new Str(value);
  }

  static auto ValueOf(Value* rhs) -> Str*;
};
}  // namespace gel

namespace fmt {
template <>
struct formatter<gel::Str> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Str& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "\"{}\"", value.str());
  }
};
}  // namespace fmt

#endif  // GEL_STR_H
