#include <gtest/gtest.h>

#include "gel/assembler.h"
#include "gel/assembler_base.h"
#include "gel/bytecode.h"
#include "gel/common.h"
#include "gtest/gtest.h"

namespace gel {
using namespace ::testing;

class AssemblerTest : public Test {
  DEFINE_NON_COPYABLE_TYPE(AssemblerTest);

 protected:
  static constexpr const auto kBytecodeOffset = 0;
  static constexpr const auto kImmediateOffset = kBytecodeOffset + sizeof(RawBytecode);
  static constexpr const auto kClassOffset = kBytecodeOffset + sizeof(RawBytecode);

 private:
  Assembler assembler_{};

 protected:
  AssemblerTest() = default;

  inline auto assembler() -> Assembler& {
    return assembler_;
  }

  inline auto assembler() const -> const Assembler& {
    return assembler_;
  }

  inline auto buffer() -> AssemblerBuffer& {
    return assembler().buffer();
  }

  inline auto cbuffer() const -> const AssemblerBuffer& {
    return assembler().cbuffer();
  }

  template <typename T>
  inline auto LoadAt(const uword idx) const -> T {
    return assembler().cbuffer().LoadAt<T>(idx);
  }

  inline auto IsBytecodeAt(const uword idx, const Bytecode expected) const -> AssertionResult {
    ASSERT(idx >= 0 && idx <= cbuffer().GetSize());
    const auto actual = LoadAt<RawBytecode>(idx);
    if (expected != actual)
      return AssertionFailure() << "expected Bytecode at " << idx << " (" << actual << " " << static_cast<uword>(actual)
                                << ") to be " << expected;
    return AssertionSuccess() << "Bytecode at " << idx << " is a " << expected;
  }

  inline auto IsBytecode(const Bytecode expected) const -> AssertionResult {
    return IsBytecodeAt(kBytecodeOffset, expected);
  }

  template <typename T>
  inline auto IsAt(const uword idx, const T expected) const -> AssertionResult {
    ASSERT(idx >= 0 && idx <= cbuffer().GetSize());
    const auto actual = LoadAt<T>(idx);
    if (expected != actual)
      return AssertionFailure() << "expected " << typeid(T).name() << " at " << idx << " (" << actual
                                << ") to equal: " << expected;
    return AssertionSuccess() << typeid(T).name() << " at " << idx << " is equal to: " << expected;
  }

  template <typename T>
  inline auto IsImmediate(const T expected) const -> AssertionResult {
    return IsAt(kImmediateOffset, expected);
  }

  template <class T>
  inline auto IsPointerAt(const uword idx, T* expected) const -> AssertionResult {
    ASSERT(expected);
    ASSERT(idx >= 0 && idx <= cbuffer().GetSize());
    const auto actual = (T*)LoadAt<uword>(idx);
    if (!actual)
      return AssertionFailure() << "expected ptr at " << idx << " (null) to equal " << expected;
    if (!actual->Equals(expected))
      return AssertionFailure() << "expected ptr at " << idx << " (" << actual << ") to equal " << expected;
    return AssertionSuccess() << "ptr at " << idx << " equals " << expected;
  }

 public:
  ~AssemblerTest() override = default;

  void SetUp() override {
    assembler_.buffer().Clear();
  }
};

#define __ assembler().
TEST_F(AssemblerTest, Test_nop) {
  __ nop();
  ASSERT_TRUE(IsBytecode(Bytecode::kNop));
}

TEST_F(AssemblerTest, Test_pop) {
  __ pop();
  ASSERT_TRUE(IsBytecode(Bytecode::kPop));
}

TEST_F(AssemblerTest, Test_dup) {
  __ dup();
  ASSERT_TRUE(IsBytecode(Bytecode::kDup));
}

TEST_F(AssemblerTest, Test_pushn) {
  __ pushn();
  ASSERT_TRUE(IsBytecode(Bytecode::kPushN));
}

TEST_F(AssemblerTest, Test_pusht) {
  __ pusht();
  ASSERT_TRUE(IsBytecode(Bytecode::kPushT));
}

TEST_F(AssemblerTest, Test_pushf) {
  __ pushf();
  ASSERT_TRUE(IsBytecode(Bytecode::kPushF));
}

TEST_F(AssemblerTest, Test_pushi) {
  static constexpr const int32_t kExpectedValue = 12987390;
  __ pushl(kExpectedValue);
  ASSERT_TRUE(IsBytecode(Bytecode::kPushI));
  ASSERT_TRUE(IsImmediate<int32_t>(kExpectedValue));
}

TEST_F(AssemblerTest, Test_pushq) {
  const auto value = String::New("Hello World");
  __ pushq(value->GetStartingAddress());
  ASSERT_TRUE(IsBytecode(Bytecode::kPushQ));
  ASSERT_TRUE(IsPointerAt(sizeof(RawBytecode), value));
}

TEST_F(AssemblerTest, Test_LoadLocal) {
  static constexpr const uword kLocalIndex = 12902;
  __ LoadLocal(kLocalIndex);
  ASSERT_TRUE(IsBytecode(Bytecode::kLoadLocal));
  ASSERT_TRUE(IsImmediate<uword>(kLocalIndex));
}

TEST_F(AssemblerTest, Test_LoadLocal0) {
  __ LoadLocal(0);
  ASSERT_TRUE(IsBytecode(Bytecode::kLoadLocal0));
}

TEST_F(AssemblerTest, Test_LoadLocal1) {
  __ LoadLocal(1);
  ASSERT_TRUE(IsBytecode(Bytecode::kLoadLocal1));
}

TEST_F(AssemblerTest, Test_LoadLocal2) {
  __ LoadLocal(2);
  ASSERT_TRUE(IsBytecode(Bytecode::kLoadLocal2));
}

TEST_F(AssemblerTest, Test_LoadLocal3) {
  __ LoadLocal(3);
  ASSERT_TRUE(IsBytecode(Bytecode::kLoadLocal3));
}

TEST_F(AssemblerTest, Test_StoreLocal) {
  static constexpr const uword kLocalIndex = 12902;
  __ StoreLocal(kLocalIndex);
  ASSERT_TRUE(IsBytecode(Bytecode::kStoreLocal));
  ASSERT_TRUE(IsImmediate<uword>(kLocalIndex));
}

TEST_F(AssemblerTest, Test_StoreLocal0) {
  __ StoreLocal(0);
  ASSERT_TRUE(IsBytecode(Bytecode::kStoreLocal0));
}

TEST_F(AssemblerTest, Test_StoreLocal1) {
  __ StoreLocal(1);
  ASSERT_TRUE(IsBytecode(Bytecode::kStoreLocal1));
}

TEST_F(AssemblerTest, Test_StoreLocal2) {
  __ StoreLocal(2);
  ASSERT_TRUE(IsBytecode(Bytecode::kStoreLocal2));
}

TEST_F(AssemblerTest, Test_StoreLocal3) {
  __ StoreLocal(3);
  ASSERT_TRUE(IsBytecode(Bytecode::kStoreLocal3));
}

TEST_F(AssemblerTest, Test_InvokeDynamic) {
  static constexpr const int32_t kNumberOfArgs = 13;
  __ invokedynamic(kNumberOfArgs);
  ASSERT_TRUE(IsBytecode(Bytecode::kInvokeDynamic));
  ASSERT_TRUE(IsImmediate<int32_t>(kNumberOfArgs));
}

TEST_F(AssemblerTest, Test_InvokeNative) {
  static constexpr const int32_t kNumberOfArgs = 13;
  __ invokenative(kNumberOfArgs);
  ASSERT_TRUE(IsBytecode(Bytecode::kInvokeNative));
  ASSERT_TRUE(IsImmediate<int32_t>(kNumberOfArgs));
}

TEST_F(AssemblerTest, Test_Ret) {
  __ ret();
  ASSERT_TRUE(IsBytecode(Bytecode::kRet));
}

TEST_F(AssemblerTest, Test_New) {
  static constexpr const auto kArgCountOffset = kClassOffset + sizeof(uword);

  static const auto kClass = String::GetClass();
  ASSERT(kClass);
  static constexpr const auto kNumberOfArgs = 141;
  __ New(kClass, kNumberOfArgs);
  ASSERT_TRUE(IsBytecode(Bytecode::kNew));
  ASSERT_TRUE(IsPointerAt(kClassOffset, kClass));
  ASSERT_TRUE(IsAt<int32_t>(kArgCountOffset, kNumberOfArgs));
}

TEST_F(AssemblerTest, Test_Cast) {
  static const auto kClass = String::GetClass();
  ASSERT(kClass);
  __ CastTo(kClass);
  ASSERT_TRUE(IsBytecode(Bytecode::kCast));
  ASSERT_TRUE(IsPointerAt(kClassOffset, kClass));
}

TEST_F(AssemblerTest, Test_Throw) {
  __ th();
  ASSERT_TRUE(IsBytecode(Bytecode::kThrow));
}

TEST_F(AssemblerTest, Test_Jump) {
  static constexpr const word kJumpPos = 1241;
  Label label(kJumpPos);
  __ jmp(&label);
  ASSERT_TRUE(IsBytecode(Bytecode::kJump));
  ASSERT_TRUE(IsImmediate<word>(kJumpPos));
}

TEST_F(AssemblerTest, Test_Jz) {
  static constexpr const word kJumpPos = 1241;
  Label label(kJumpPos);
  __ jz(&label);
  ASSERT_TRUE(IsBytecode(Bytecode::kJz));
  ASSERT_TRUE(IsImmediate<word>(kJumpPos));
}

TEST_F(AssemblerTest, Test_Jnz) {
  static constexpr const word kJumpPos = 1241;
  Label label(kJumpPos);
  __ jnz(&label);
  ASSERT_TRUE(IsBytecode(Bytecode::kJnz));
  ASSERT_TRUE(IsImmediate<word>(kJumpPos));
}

TEST_F(AssemblerTest, Test_Jne) {
  static constexpr const word kJumpPos = 1241;
  Label label(kJumpPos);
  __ jne(&label);
  ASSERT_TRUE(IsBytecode(Bytecode::kJne));
  ASSERT_TRUE(IsImmediate<word>(kJumpPos));
}

TEST_F(AssemblerTest, Test_Jeq) {
  static constexpr const word kJumpPos = 1241;
  Label label(kJumpPos);
  __ jeq(&label);
  ASSERT_TRUE(IsBytecode(Bytecode::kJeq));
  ASSERT_TRUE(IsImmediate<word>(kJumpPos));
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
TEST_F(AssemblerTest, Test_BranchNotEqual) {
  static constexpr const word kJumpPos = 1241;
  Label equals_zero;
  __ pushl(10);
  __ pushl(11);
  __ sub();
  __ jeq(&equals_zero);
  __ pushl(1);
  __ pushl(2);
  __ add();
  __ Bind(&equals_zero);
  __ ret();
  enum Layout : uword {
    kStartOffset = 0,
    KSimpleInstr0Length = sizeof(RawBytecode),
    kSimpleInstr1Length = sizeof(RawBytecode) + sizeof(uword),
    kComplexInstrLength = sizeof(RawBytecode) + sizeof(word),
    // pushl 10
    kFirstInstrOffset = kStartOffset,
    kFirstInstrLength = kSimpleInstr1Length,
    // pushl 11
    kSecondInstrOffset = kFirstInstrOffset + kFirstInstrLength,
    kSecondInstrLength = kSimpleInstr1Length,
    // sub
    kThirdInstrOffset = kSecondInstrOffset + kSecondInstrLength,
    kThirdInstrLength = KSimpleInstr0Length,
    // jeq equals_zero
    kFourthInstrOffset = kThirdInstrOffset + kThirdInstrLength,  // #19
    kFourthInstrLength = kComplexInstrLength,
    // pushl 1
    kFifthInstrOffset = kFourthInstrOffset + kFourthInstrLength,
    kFifthInstrLength = kSimpleInstr1Length,
    // pushl 2
    kSixthInstrOffset = kFifthInstrOffset + kFifthInstrLength,
    kSixthInstrLength = kSimpleInstr1Length,
    // add
    kSeventhInstrOffset = kSixthInstrOffset + kSixthInstrLength,
    kSeventhInstrLength = KSimpleInstr0Length,
    // ret
    kEightInstrOffset = kSeventhInstrOffset + kSeventhInstrLength,  // #47
    kEightInstrLength = KSimpleInstr0Length,
    // total length of instructions
    kTotalLength = kFirstInstrLength + kSecondInstrLength + kThirdInstrLength + kFourthInstrLength + kFifthInstrLength +
                   kSixthInstrLength + kSeventhInstrLength + kEightInstrLength,
  };
  ASSERT_EQ(kTotalLength, cbuffer().GetSize());
  // pushl 10
  ASSERT_TRUE(IsBytecodeAt(kFirstInstrOffset, Bytecode::kPushI));
  ASSERT_TRUE(IsAt<uword>(kFirstInstrOffset + sizeof(RawBytecode), 10));
  // pushl 11
  ASSERT_TRUE(IsBytecodeAt(kSecondInstrOffset, Bytecode::kPushI));
  ASSERT_TRUE(IsAt<uword>(kSecondInstrOffset + sizeof(RawBytecode), 11));
  // sub
  ASSERT_TRUE(IsBytecodeAt(kThirdInstrOffset, Bytecode::kSubtract));
  // jeq equals_zero
  ASSERT_TRUE(IsBytecodeAt(kFourthInstrOffset, Bytecode::kJeq));
  ASSERT_EQ(LoadAt<word>(kFourthInstrOffset + sizeof(RawBytecode)),
            equals_zero.GetPos() - (kFourthInstrOffset + sizeof(RawBytecode)));
  // pushl 1
  ASSERT_TRUE(IsBytecodeAt(kFifthInstrOffset, Bytecode::kPushI));
  ASSERT_TRUE(IsAt<uword>(kFifthInstrOffset + sizeof(RawBytecode), 1));
  // pushl 2
  ASSERT_TRUE(IsBytecodeAt(kSixthInstrOffset, Bytecode::kPushI));
  ASSERT_TRUE(IsAt<uword>(kSixthInstrOffset + sizeof(RawBytecode), 2));
  // add
  ASSERT_TRUE(IsBytecodeAt(kSeventhInstrOffset, Bytecode::kAdd));
  // ret
  ASSERT_TRUE(IsBytecodeAt(kEightInstrOffset, Bytecode::kRet));
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
#undef __
}  // namespace gel