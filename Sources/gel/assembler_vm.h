#include "gel/platform.h"
#ifndef GEL_ASSEMBLER_H
#error "Please #include <gel/assembler.h> instead."
#endif  // GEL_ASSEMBLER_H

#ifndef GEL_ASSEMBLER_VM_H
#define GEL_ASSEMBLER_VM_H

#include "gel/assembler_base.h"
#include "gel/bytecode.h"
#include "gel/common.h"
#include "gel/expression.h"
#include "gel/to_string_helper.h"

namespace gel {
using namespace vm;

class NativeProcedure;
class Assembler {
  friend class AssemblerTest;
  DEFINE_NON_COPYABLE_TYPE(Assembler);

 private:
  AssemblerBuffer buffer_{};

  auto buffer() -> AssemblerBuffer& {
    return buffer_;
  }

 public:
  Assembler() = default;
  ~Assembler() = default;

  auto cbuffer() const -> const AssemblerBuffer& {
    return buffer_;
  }

  inline void EmitOp(const Bytecode::Op op) {
    buffer().Emit<Bytecode::Op>(op);
  }

  void EmitImmediate(const uword value);
  void EmitLabel(Label* label);
  void EmitLabelLink(Label* label);
  void Jump(Bytecode::Op op, Label* label);

  void Bind(Label* label);

  void jmp(Label* label) {
    ASSERT(label);
    return Jump(Bytecode::kJump, label);
  }

  void jz(Label* label) {
    ASSERT(label);
    return Jump(Bytecode::kJz, label);
  }

  void jnz(Label* label) {
    ASSERT(label);
    return Jump(Bytecode::kJnz, label);
  }

  void jne(Label* label) {
    ASSERT(label);
    return Jump(Bytecode::kJne, label);
  }

  void jeq(Label* label) {
    ASSERT(label);
    return Jump(Bytecode::kJeq, label);
  }

  template <class T>
  inline void EmitAddress(const T* value) {
    return Emit(value->GetStartingAddress());
  }

  inline void Emit(const uword value) {
    buffer().Emit<uword>(value);
  }

  inline void CastTo(Class* cls) {
    EmitOp(Bytecode::kCast);
    EmitAddress(cls);
  }

  inline void dup() {
    return EmitOp(Bytecode::kDup);
  }

  inline void nop() {
    return EmitOp(Bytecode::kNop);
  }

  inline void pop() {
    return EmitOp(Bytecode::kPop);
  }

  inline void ret() {
    return EmitOp(Bytecode::kRet);
  }

  inline void pushq(const uword value) {
    EmitOp(Bytecode::kPushQ);
    Emit(value);
  }

  inline void pushl(const uword rhs) {
    EmitOp(Bytecode::kPushI);
    Emit(rhs);
  }

  inline void pusht() {
    EmitOp(Bytecode::kPushT);
  }

  inline void pushf() {
    EmitOp(Bytecode::kPushF);
  }

  inline void pushn() {
    EmitOp(Bytecode::kPushN);
  }

  template <class T>
  inline void Push(T* value) {
    ASSERT(value);
    if (gel::IsNull(value))
      return pushn();
    if (value->IsLong())
      return pushl(value->AsLong()->Get());
    else if (value->IsBool() && value->AsBool()->Get())
      return pusht();
    else if (value->IsBool() && !value->AsBool()->Get())
      return pushf();
    return pushq(value->GetStartingAddress());
  }

  inline void lookup() {
    EmitOp(Bytecode::kLookup);
  }

  inline void invoke(Lambda* func, const uword num_args) {
    ASSERT(func);
    EmitOp(Bytecode::kInvoke);
    EmitAddress(func);
    Emit(num_args);
  }

  inline void invokedynamic(const uword num_args) {
    EmitOp(Bytecode::kInvokeDynamic);
    Emit(num_args);
  }

  inline void invokenative(Procedure* func, const uword num_args) {
    ASSERT(func);
    EmitOp(Bytecode::kInvokeNative);
    EmitAddress(func);
    Emit(num_args);
  }

  void th() {
    return EmitOp(Bytecode::kThrow);
  }

  inline void LoadLocal(const uword idx) {
    static constexpr const auto kTotalLLInstrs = Bytecode::kLoadLocal3 - Bytecode::kLoadLocal0;
    ASSERT(idx >= 0);
    if (idx <= kTotalLLInstrs)
      return EmitOp(static_cast<Bytecode::Op>(Bytecode::kLoadLocal0 + idx));
    EmitOp(Bytecode::kLoadLocal);
    Emit(idx);
  }

  inline void StoreLocal(const uword idx) {
    static constexpr const auto kTotalSLInstrs = Bytecode::kStoreLocal3 - Bytecode::kStoreLocal0;
    ASSERT(idx >= 0);
    if (idx <= kTotalSLInstrs)
      return EmitOp(static_cast<Bytecode::Op>(Bytecode::kStoreLocal0 + idx));
    EmitOp(Bytecode::kStoreLocal);
    Emit(idx);
  }

  inline void negate() {
    return EmitOp(Bytecode::kNot);
  }

  inline void car() {
    return EmitOp(Bytecode::kCar);
  }

  inline void cdr() {
    return EmitOp(Bytecode::kCdr);
  }

  inline void isnonnull() {
    return EmitOp(Bytecode::kNonnull);
  }

  inline void isnull() {
    return EmitOp(Bytecode::kNull);
  }

  inline void add() {
    return EmitOp(Bytecode::kAdd);
  }

  inline void sub() {
    return EmitOp(Bytecode::kSubtract);
  }

  inline void mul() {
    return EmitOp(Bytecode::kMultiply);
  }

  inline void div() {
    return EmitOp(Bytecode::kDivide);
  }

  inline void mod() {
    return EmitOp(Bytecode::kModulus);
  }

  inline void eq() {
    return EmitOp(Bytecode::kEquals);
  }

  inline void band() {
    return EmitOp(Bytecode::kBinaryAnd);
  }

  inline void bor() {
    return EmitOp(Bytecode::kBinaryOr);
  }

  inline void gt() {
    return EmitOp(Bytecode::kGreaterThan);
  }

  inline void gte() {
    return EmitOp(Bytecode::kGreaterThanEqual);
  }

  inline void lt() {
    return EmitOp(Bytecode::kLessThan);
  }

  inline void lte() {
    return EmitOp(Bytecode::kLessThanEqual);
  }

  inline void cons() {
    return EmitOp(Bytecode::kCons);
  }

  inline void instanceof() {
    return EmitOp(Bytecode::kInstanceOf);
  }

  inline void CheckInstance(Class* cls) {
    ASSERT(cls);
    EmitOp(Bytecode::kCheckInstance);
    EmitAddress(cls);
  }

  inline void New(Class* cls, const uword num_args = 0) {
    ASSERT(cls);
    EmitOp(Bytecode::kNew);
    EmitAddress(cls);
    Emit(num_args);
  }

  auto Assemble() const -> Region;
};
}  // namespace gel

#endif  // GEL_ASSEMBLER_VM_H