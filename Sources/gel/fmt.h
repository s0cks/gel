#ifndef GEL_FMT_H
#define GEL_FMT_H

#include <fmt/format.h>

#include "gel/object.h"

namespace fmt {
template <>
struct formatter<gel::Object> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Object& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.ToString());
  }
};
}  // namespace fmt

#endif  // GEL_FMT_H
