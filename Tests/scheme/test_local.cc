#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "gtest/gtest.h"
#include "scheme/common.h"
#include "scheme/local.h"
#include "scheme/local_assertions.h"
#include "scheme/local_scope.h"
#include "scheme/mock_local_scope.h"
#include "scheme/type.h"
#include "scheme/type_assertions.h"

namespace scm {
using namespace ::testing;

class LocalVariableTest : public Test {};

TEST_F(LocalVariableTest, Test_New_NoValue) {  // NOLINT
  static constexpr const auto kSymbolValue = "test";
  const auto symbol = Symbol::New(kSymbolValue);
  ASSERT_TRUE(IsSymbol(symbol, kSymbolValue));

  MockLocalScope scope;
  EXPECT_CALL(scope, GetNumberOfLocals());

  const auto local = LocalVariable::New(&scope, symbol);
  ASSERT_TRUE(local);
  ASSERT_TRUE(IsLocal(local, kSymbolValue, 0));
  ASSERT_TRUE(HasOwner(local, &scope));
  ASSERT_TRUE(HasNoValue(local));
}

TEST_F(LocalVariableTest, Test_New_WithValue) {  // NOLINT
  static constexpr const auto kSymbolValue = "test";
  const auto symbol = Symbol::New(kSymbolValue);
  ASSERT_TRUE(IsSymbol(symbol, kSymbolValue));

  const auto value = Bool::True();
  ASSERT_TRUE(IsTrue(value));

  MockLocalScope scope;
  EXPECT_CALL(scope, GetNumberOfLocals());

  const auto local = LocalVariable::New(&scope, symbol, value);
  ASSERT_TRUE(local);
  ASSERT_TRUE(IsLocal(local, kSymbolValue, 0));
  ASSERT_TRUE(HasOwner(local, &scope));
  ASSERT_TRUE(HasValue(local, value));
}
}  // namespace scm