#ifndef GEL_TEST_ASSEMBLER_H
#define GEL_TEST_ASSEMBLER_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "assembler.h"
#include "common.h"
#include "compiled_code.h"

namespace gel {
using namespace ::testing;

class AssemblerTest : public Test {
  DEFINE_NON_COPYABLE_TYPE(AssemblerTest);

 private:
  Assembler assm_{};

 public:
  AssemblerTest() = default;
  ~AssemblerTest() override = default;

  auto assembler() -> Assembler* {
    return &assm_;
  }

  auto assembler_buffer() -> AssemblerBuffer& {
    return assembler()->buffer();
  }

  void SetUp() override {
    assembler_buffer().Clear();
  }
};

MATCHER_P(IsBytecode, op, "") {  // NOLINT
  return *((Bytecode::Op*)arg) == op;
}

#define __ assembler()->

#define COMPILE(Name)                                 \
  const auto Name = CompiledCode::New(__ Assemble()); \
  ASSERT_TRUE((Name)->IsCompiled());

static inline auto OperandAddress(CompiledCode* code) -> uword {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  return *(uword*)(code->GetStartingAddress() + sizeof(Bytecode::Op));
}

TEST_F(AssemblerTest, Test_CheckInstance) {
  const auto cls = Long::GetClass();
  ASSERT(cls);
  __ CheckInstance(cls);
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kCheckInstance));
  ASSERT_EQ(OperandAddress(code), cls->GetStartingAddress());
}

static inline auto OperandArgs(CompiledCode* code) -> uword {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  return *(uword*)(code->GetStartingAddress() + sizeof(Bytecode::Op) + sizeof(uword));
}

TEST_F(AssemblerTest, Test_New) {
  static constexpr const auto kNumberOfArgs = 1;
  const auto cls = Long::GetClass();
  ASSERT(cls);
  __ New(cls, kNumberOfArgs);
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kNew));
  ASSERT_EQ(OperandAddress(code), cls->GetStartingAddress());
  ASSERT_EQ(OperandArgs(code), kNumberOfArgs);
}

TEST_F(AssemblerTest, Test_StoreField) {
  const auto field = Pair::kFirstField;
  __ stfield(field);
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kStoreField));
  ASSERT_EQ(OperandAddress(code), field->GetStartingAddress());
}

TEST_F(AssemblerTest, Test_LoadField) {
  const auto field = Pair::kFirstField;
  __ ldfield(field);
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kLoadField));
  ASSERT_EQ(OperandAddress(code), field->GetStartingAddress());
}

TEST_F(AssemblerTest, Test_Dup) {
  __ dup();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kDup));
}

TEST_F(AssemblerTest, Test_Pop) {
  __ pop();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kPop));
}

TEST_F(AssemblerTest, Test_Nop) {
  __ nop();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kNop));
}

TEST_F(AssemblerTest, Test_Ret) {
  __ ret();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kRet));
}

static inline auto OperandValue(CompiledCode* code) -> word {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  return *(word*)(code->GetStartingAddress() + sizeof(Bytecode::Op));
}

TEST_F(AssemblerTest, Test_PushI_Neg) {
  __ pushl(-123);
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kPushI));
  ASSERT_EQ(OperandValue(code), -123);
}

TEST_F(AssemblerTest, Test_PushI_Pos) {
  __ pushl(555);
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kPushI));
  ASSERT_EQ(OperandValue(code), 555);
}

TEST_F(AssemblerTest, Test_PushTrue) {
  __ pusht();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kPushT));
}

TEST_F(AssemblerTest, Test_PushFalse) {
  __ pushf();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kPushF));
}

TEST_F(AssemblerTest, Test_PushNull) {
  __ pushn();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kPushN));
}

TEST_F(AssemblerTest, Test_Throw) {
  __ th();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kThrow));
}

TEST_F(AssemblerTest, Test_Negate) {
  __ negate();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kNot));
}

TEST_F(AssemblerTest, Test_Car) {
  __ car();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kCar));
}

TEST_F(AssemblerTest, Test_Cdr) {
  __ cdr();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kCdr));
}

TEST_F(AssemblerTest, Test_IsNonnull) {
  __ isnonnull();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kNonnull));
}

TEST_F(AssemblerTest, Test_IsNull) {
  __ isnull();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kNull));
}

TEST_F(AssemblerTest, Test_Add) {
  __ add();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kAdd));
}

TEST_F(AssemblerTest, Test_Sub) {
  __ sub();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kSubtract));
}

TEST_F(AssemblerTest, Test_Divide) {
  __ div();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kDivide));
}

TEST_F(AssemblerTest, Test_Multiply) {
  __ mul();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kMultiply));
}

TEST_F(AssemblerTest, Test_Modulus) {
  __ mod();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kModulus));
}

TEST_F(AssemblerTest, Test_Eq) {
  __ eq();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kEq));
}

TEST_F(AssemblerTest, Test_BitAnd) {
  __ band();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kBitAnd));
}

TEST_F(AssemblerTest, Test_BitOr) {
  __ bor();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kBitOr));
}

TEST_F(AssemblerTest, Test_BitXor) {
  __ bxor();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kBitXor));
}

TEST_F(AssemblerTest, Test_GreaterThan) {
  __ gt();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kGreaterThan));
}

TEST_F(AssemblerTest, Test_GreaterThanEqual) {
  __ gte();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kGreaterThanEqual));
}

TEST_F(AssemblerTest, Test_LessThan) {
  __ lt();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kLessThan));
}

TEST_F(AssemblerTest, Test_LessThanEqual) {
  __ lte();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kLessThanEqual));
}

TEST_F(AssemblerTest, Test_Shl) {
  __ shl();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kShiftLeft));
}

TEST_F(AssemblerTest, Test_Shr) {
  __ shr();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kShiftRight));
}

TEST_F(AssemblerTest, Test_Cons) {
  __ cons();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kCons));
}

TEST_F(AssemblerTest, Test_InstanceOf) {
  __ instanceof();
  COMPILE(code);
  ASSERT_THAT(code->GetStartingAddress(), IsBytecode(Bytecode::kInstanceOf));
}

#undef __
}  // namespace gel

#endif  // GEL_TEST_ASSEMBLER_H
