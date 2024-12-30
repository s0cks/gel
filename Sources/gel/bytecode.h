#ifndef GEL_BYTECODE_H
#define GEL_BYTECODE_H

#include "gel/common.h"
#include "gel/expression.h"  //TODO: remove include
#include "gel/platform.h"

#define FOR_EACH_BYTECODE(V) \
  V(Nop)                     \
  V(Pop)                     \
  V(Dup)                     \
  V(Lookup)                  \
  V(StoreLocal)              \
  V(StoreLocal0)             \
  V(StoreLocal1)             \
  V(StoreLocal2)             \
  V(StoreLocal3)             \
  V(LoadLocal)               \
  V(LoadLocal0)              \
  V(LoadLocal1)              \
  V(LoadLocal2)              \
  V(LoadLocal3)              \
  V(Invoke)                  \
  V(InvokeDynamic)           \
  V(InvokeNative)            \
  V(CheckInstance)           \
  V(Ret)                     \
  V(PushQ)                   \
  V(PushI)                   \
  V(PushN)                   \
  V(PushT)                   \
  V(PushF)                   \
  V(Jump)                    \
  V(Jz)                      \
  V(Jnz)                     \
  V(Jeq)                     \
  V(Jne)                     \
  V(Cast)                    \
  V(New)                     \
  V(Throw)                   \
  V(LoadField)               \
  V(StoreField)              \
  FOR_EACH_UNARY_OP(V)       \
  FOR_EACH_BINARY_OP(V)

namespace gel::vm {
using RawBytecode = uint8_t;
class Bytecode {
  DEFINE_DEFAULT_COPYABLE_TYPE(Bytecode);

 public:
  enum Op : RawBytecode {
    kInvalid = 0x0,
#define DEFINE_OP(Name) k##Name,
    FOR_EACH_BYTECODE(DEFINE_OP)
#undef DEFINE_OP
        kTotalNumberOfOps,
  };
  static_assert(kTotalNumberOfOps <= UINT8_MAX, "Expected kTotalNumberOfOps to be less than the max value of uint8_t.");

 private:
  RawBytecode raw_;

 public:
  constexpr Bytecode(const RawBytecode value = kInvalid) :
    raw_(value) {}
  ~Bytecode() = default;

  constexpr auto raw() const -> RawBytecode {
    return raw_;
  }

  constexpr auto op() const -> Op {
    return static_cast<Op>(raw());
  }

  inline constexpr auto IsInvalidOp() const -> bool {
    return op() == kInvalid;
  }

#define DEFINE_OP_CHECK(Name)                        \
  inline constexpr auto Is##Name##Op() const->bool { \
    return op() == k##Name;                          \
  }
  FOR_EACH_BYTECODE(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK

  inline constexpr auto IsBinaryOp() const -> bool {
    switch (op()) {
#define DEFINE_OP_CHECK(Name) \
  case Bytecode::k##Name:     \
    return true;
      FOR_EACH_BINARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK
      default:
        return false;
    }
  }

  inline constexpr auto IsUnaryOp() const -> bool {
    switch (op()) {
#define DEFINE_OP_CHECK(Name) \
  case Bytecode::k##Name:     \
    return true;
      FOR_EACH_UNARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK
      default:
        return false;
    }
  }

  constexpr auto mnemonic() const -> const char* {
    switch (op()) {
      case kNop:
        return "nop";
      case kDup:
        return "dup";
      case kPop:
        return "pop";
      case kStoreLocal:
        return "sl";
      case kStoreLocal0:
        return "sl0";
      case kStoreLocal1:
        return "sl1";
      case kStoreLocal2:
        return "sl2";
      case kStoreLocal3:
        return "sl3";
      case kLoadLocal:
        return "ll";
      case kLoadLocal0:
        return "ll0";
      case kLoadLocal1:
        return "ll1";
      case kLoadLocal2:
        return "ll2";
      case kLoadLocal3:
        return "ll3";
      case kInvoke:
        return "invoke";
      case kInvokeDynamic:
        return "invokedynamic";
      case kInvokeNative:
        return "invokenative";
      case kRet:
        return "ret";
      case kThrow:
        return "th";
      case kPushQ:
        return "pushq";
      case kPushI:
        return "pushi";
      case kPushN:
        return "pushn";
      case kPushT:
        return "pusht";
      case kPushF:
        return "pushf";
      case kStoreField:
        return "stfield";
      case kLoadField:
        return "ldfield";
      case kNot:
        return "negate";
      case kCar:
        return "car";
      case kCdr:
        return "cdr";
      case kLookup:
        return "lookup";
      case kNonnull:
        return "neqnull";
      case kNull:
        return "eqnull";
      case kAdd:
        return "add";
      case kSubtract:
        return "sub";
      case kMultiply:
        return "mul";
      case kDivide:
        return "div";
      case kModulus:
        return "mod";
      case kEquals:
        return "eq";
      case kBinaryAnd:
        return "band";
      case kBinaryOr:
        return "bor";
      case kGreaterThan:
        return "gt";
      case kGreaterThanEqual:
        return "gte";
      case kLessThan:
        return "lt";
      case kLessThanEqual:
        return "lte";
      case kCons:
        return "cons";
      case kInstanceOf:
        return "instanceof";
      case kJump:
        return "jmp";
      case kJz:
        return "jz";
      case kJnz:
        return "jnz";
      case kJeq:
        return "jeq";
      case kJne:
        return "jne";
      case kCheckInstance:
        return "checkinstance";
      case kNew:
        return "new";
      case kCast:
        return "cast";
      case kInvalid:
      default:
        return "unknown";
    }
  }

  auto operator=(const RawBytecode& rhs) -> Bytecode& {
    raw_ = rhs;
    return *this;
  }

  auto operator==(const Bytecode& rhs) const -> bool {
    return raw() == rhs.raw();
  }

  auto operator==(const RawBytecode& rhs) const -> bool {
    return raw() == rhs;
  }

  auto operator!=(const Bytecode& rhs) const -> bool {
    return raw() != rhs.raw();
  }

  auto operator!=(const RawBytecode& rhs) const -> bool {
    return raw() != rhs;
  }

  constexpr operator RawBytecode() const {
    return raw();
  }

  friend auto operator<<(std::ostream& stream, const Bytecode& rhs) -> std::ostream& {
    switch (rhs.raw()) {
#define DEFINE_TO_STRING(Name) \
  case k##Name:                \
    return stream << #Name;
      FOR_EACH_BYTECODE(DEFINE_TO_STRING)
#undef DEFINE_TO_STRING
      case kInvalid:
      default:
        return stream << "Unknown gel::Bytecode(" << rhs.raw() << ")";
    }
  }
};
static_assert(sizeof(Bytecode) == sizeof(uint8_t), "expected sizeof(Bytecode) to equal sizeof(uint8_t).");
}  // namespace gel::vm

#endif  // GEL_BYTECODE_H
