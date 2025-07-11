#ifndef GEL_UNARY_OP_H
#define GEL_UNARY_OP_H

#include <cstdint>
#include <ostream>

namespace gel {
#define FOR_EACH_UNARY_OP(V) \
  V(Not)                     \
  V(Car)                     \
  V(Cdr)                     \
  V(Nonnull)                 \
  V(Null)                    \
  V(BitNot)

enum UnaryOp : uint64_t {
#define DEFINE_UNARY_OP(Name) k##Name,
  FOR_EACH_UNARY_OP(DEFINE_UNARY_OP)
#undef DEFINE_UNARY_OP
};

static inline constexpr auto ToString(const UnaryOp& rhs) -> std::string_view {
  switch (rhs) {
#define DEFINE_TO_STRING(Name) \
  case UnaryOp::k##Name:       \
    return #Name;
    FOR_EACH_UNARY_OP(DEFINE_TO_STRING)
#undef DEFINE_TO_STRING
    default:
      return "Unknown UnaryOp";
  }
}

static inline auto operator<<(std::ostream& stream, const UnaryOp& rhs) -> std::ostream& {
  return stream << ToString(rhs);
}
}  // namespace gel

#endif  // GEL_UNARY_OP_H
