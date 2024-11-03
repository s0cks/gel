#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "scheme/object.h"

namespace scm {
using namespace ::testing;

class BoolTest : public Test {};

TEST_F(BoolTest, Test_New) {  // NOLINT
  const auto v1 = Bool::New(true);
  ASSERT_TRUE(v1);
  ASSERT_TRUE(v1->Get());
  const auto v2 = Bool::New(false);
  ASSERT_TRUE(v2);
  ASSERT_FALSE(v2->Get());
}
}  // namespace scm