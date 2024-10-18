#include <gtest/gtest.h>

#include "scheme/type.h"

namespace scm {
using namespace ::testing;

class NumberTest : public Test {};

TEST_F(NumberTest, Test_New) {  // NOLINT
  static constexpr const uintptr_t kValue = 99;
  const auto val = Number::New(kValue);
  ASSERT_TRUE(val);
  ASSERT_EQ(val->GetValue(), kValue);
}
}  // namespace scm