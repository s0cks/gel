#ifndef GEL_BINARY_OP_H
#define GEL_BINARY_OP_H

#include <ostream>

namespace gel {
#define FOR_EACH_BINARY_OP(V) \
  V(Add)                      \
  V(Subtract)                 \
  V(Multiply)                 \
  V(Divide)                   \
  V(Modulus)                  \
  V(Eq)                       \
  V(ShiftLeft)                \
  V(ShiftRight)               \
  V(BitAnd)                   \
  V(BitOr)                    \
  V(BitXor)                   \
  V(GreaterThan)              \
  V(GreaterThanEqual)         \
  V(LessThan)                 \
  V(LessThanEqual)            \
  V(Cons)                     \
  V(InstanceOf)

enum BinaryOp : uint64_t {
#define DEFINE_BINARY_OP(Name) k##Name,
  FOR_EACH_BINARY_OP(DEFINE_BINARY_OP)
#undef DEFINE_BINARY_OP
};

static inline constexpr auto BinaryOpToString(const BinaryOp& rhs) -> const char* {
  switch (rhs) {
#define DEFINE_TO_STRING(Name) \
  case BinaryOp::k##Name:      \
    return #Name;
    FOR_EACH_BINARY_OP(DEFINE_TO_STRING)
#undef DEFINE_TO_STRING
    default:
      return "Invalid Binary Op";
  }
}

static inline auto operator<<(std::ostream& stream, const BinaryOp& rhs) -> std::ostream& {
  switch (rhs) {
#define DEFINE_TO_STRING(Name) \
  case BinaryOp::k##Name:      \
    return stream << #Name;
    FOR_EACH_BINARY_OP(DEFINE_TO_STRING)
#undef DEFINE_TO_STRING
    default:
      return stream << "Unknown BinaryOp: " << static_cast<int64_t>(rhs);
  }
}
}  // namespace gel

#endif  // GEL_BINARY_OP_H
