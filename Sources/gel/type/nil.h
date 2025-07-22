#ifndef GEL_NIL_H
#define GEL_NIL_H

#include <fmt/base.h>

#include "gel/common.h"
#include "gel/type/value.h"

namespace gel {
class Nil : public Value {
 public:
  Nil() = default;
  ~Nil() override = default;

  DECLARE_VALUE_TYPE(Nil);

 public:
  static inline auto New() -> Nil* {
    return new Nil();
  }

  static auto Get() -> Nil*;
};
}  // namespace gel

namespace fmt {
template <>
class formatter<gel::Nil> : public fmt::formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Nil& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    if (VLOG_IS_ON(10))
      return format_to(ctx.out(), value.ToString());
    return format_to(ctx.out(), "nil");
  }
};
}  // namespace fmt

#endif  // GEL_NIL_H
