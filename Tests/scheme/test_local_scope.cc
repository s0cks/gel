#include <gtest/gtest.h>

#include "scheme/local_scope.h"

namespace scm {
using namespace ::testing;

class LocalScopeTest : public Test {  // NOLINT
 private:
  LocalScope* scope_;

 public:
  LocalScopeTest() :
    scope_(LocalScope::New()) {}
  ~LocalScopeTest() override = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
  }

  void SetUp() override {
    ASSERT_TRUE(HasScope());
  }
};

TEST_F(LocalScopeTest, Test_Has_Fails_NoSymbol) {  // NOLINT
  static constexpr const auto kTestSymbol = "test";
  ASSERT_FALSE(GetScope()->Has(kTestSymbol));
}
}  // namespace scm