#ifndef GEL_BINARY_OP_H
#define GEL_BINARY_OP_H

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
}  // namespace gel

#endif  // GEL_BINARY_OP_H
