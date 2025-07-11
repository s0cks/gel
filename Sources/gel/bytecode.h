#ifndef GEL_BYTECODE_H
#define GEL_BYTECODE_H

#include <concepts>
#include <cstdint>
#include <ios>
#include <ostream>
#include <type_traits>

#include "gel/binary_op.h"
#include "gel/common.h"
#include "gel/expr/expression.h"  // IWYU pragma: keep
#include "gel/platform.h"
#include "gel/unary_op.h"

#define FOR_EACH_BYTECODE(V) \
  V(Nop)                     \
  V(Pop)                     \
  V(Dup)                     \
  V(Dup2)                    \
  V(StoreFirst)              \
  V(StoreSecond)             \
  V(LoadFirst)               \
  V(LoadSecond)              \
  V(Lookup)                  \
  V(GetInstanceProc)         \
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
  V(BranchTrue)              \
  V(BranchFalse)             \
  V(BranchEq)                \
  V(BranchNeq)               \
  V(BranchGreaterThan)       \
  V(BranchLessThan)          \
  V(Cast)                    \
  V(New)                     \
  V(List)                    \
  V(Throw)                   \
  V(LoadField)               \
  V(StoreField)              \
  FOR_EACH_UNARY_OP(V)       \
  FOR_EACH_BINARY_OP(V)

namespace gel::vm {
enum BranchCondition : uint8_t {
  kIsTrue,
  kIsFalse,
  kEquals,
  kNotEquals,
  kGreaterThan,
  kLessThan,
};

using RawBytecode = uint8_t;
class Bytecode {
  DEFINE_DEFAULT_COPYABLE_TYPE(Bytecode);

 public:
  enum Op : RawBytecode {
    kInvalid = 0x0,
  // clang-format off
#define DEFINE_OP(Name) k##Name,
    FOR_EACH_BYTECODE(DEFINE_OP)
#undef DEFINE_OP
    kTotalNumberOfOps,
    // clang-format on
  };
  static_assert(kTotalNumberOfOps <= UINT8_MAX, "Expected kTotalNumberOfOps to be less than the max value of uint8_t.");

 private:
  RawBytecode raw_;

 public:
  constexpr Bytecode(const RawBytecode value = kInvalid) :
    raw_(value) {}
  ~Bytecode() = default;

  inline constexpr auto raw() const -> RawBytecode {
    return raw_;
  }

  inline constexpr auto op() const -> Op {
    return static_cast<Op>(raw());
  }

  inline constexpr auto IsInvalidOp() const -> bool {
    return op() == kInvalid;
  }

#define DEFINE_OP_CHECK(Name)                          \
  inline constexpr auto Is##Name##Op() const -> bool { \
    return op() == k##Name;                            \
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

  inline constexpr auto mnemonic() const -> const char* {
    switch (op()) {
      case kNop:
        return "nop";
      case kDup:
        return "dup";
      case kDup2:
        return "dup2";
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
      case kLoadFirst:
        return "ldfirst";
      case kLoadSecond:
        return "ldsecond";
      case kStoreFirst:
        return "stfirst";
      case kStoreSecond:
        return "stsecond";
      case kGetInstanceProc:
        return "ldinstanceproc";
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
      case kEq:
        return "eq";
      case kBitAnd:
        return "band";
      case kBitOr:
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
      case kBranchEq:
        return "be";
      case kBranchNeq:
        return "bne";
      case kBranchTrue:
        return "bt";
      case kBranchFalse:
        return "bf";
      case kBranchGreaterThan:
        return "bgt";
      case kBranchLessThan:
        return "blt";
      case kCheckInstance:
        return "checkinstance";
      case kNew:
        return "new";
      case kList:
        return "list";
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

  constexpr auto operator==(const Bytecode& rhs) const -> bool {
    return raw() == rhs.raw();
  }

  constexpr auto operator==(const RawBytecode& rhs) const -> bool {
    return raw() == rhs;
  }

  constexpr auto operator!=(const Bytecode& rhs) const -> bool {
    return raw() != rhs.raw();
  }

  constexpr auto operator!=(const RawBytecode& rhs) const -> bool {
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
      default: {
        stream << "Unknown gel::Bytecode(";
        PrintHex(stream, rhs.raw());
        stream << ")";
        return stream;
      }
    }
  }

 private:
  template <typename T>
  static inline void PrintHex(std::ostream& stream, const T rhs)
    requires(std::is_integral_v<T> || std::same_as<T, Bytecode::Op>)
  {
    ostream_guard guard(stream);
    stream << "0x" << std::hex << std::nouppercase << static_cast<uword>(rhs);
  }

 public:
  static inline void PrintRaw(std::ostream& stream, const Bytecode bc) {
    return PrintHex(stream, bc.raw());
  }

  static inline void PrintRaw(std::ostream& stream, const Bytecode::Op op) {
    return PrintHex(stream, op);
  }

#ifdef GEL_DEBUG

  static inline void PrintAllOps() {
    LOG(INFO) << "Bytecode Ops:";
    for (auto idx = 0; idx < Bytecode::kTotalNumberOfOps; idx++) {
      PrintHex(LOG(INFO) << " - ", static_cast<RawBytecode>(idx));
    }
  }

#endif  // GEL_DEBUG
};
static_assert(sizeof(Bytecode) == sizeof(uint8_t), "expected sizeof(Bytecode) to equal sizeof(uint8_t).");
}  // namespace gel::vm

#endif  // GEL_BYTECODE_H
