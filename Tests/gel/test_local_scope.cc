#include <gtest/gtest.h>

#include "gel/local_scope.h"
#include "gel/type_assertions.h"

namespace gel {
using namespace ::testing;

class LocalScopeTest : public Test {  // NOLINT
 private:
  LocalScope* scope_ = nullptr;

  inline void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

  inline auto RemoveScope() -> LocalScope* {
    const auto old_scope = GetScope();
    ASSERT(old_scope);
    scope_ = nullptr;
    return old_scope;
  }

 public:
  LocalScopeTest() = default;
  ~LocalScopeTest() override = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
  }

  void SetUp() override {
    ASSERT_FALSE(HasScope());
    SetScope(LocalScope::New());
    ASSERT_TRUE(HasScope());
  }

  void TearDown() override {
    ASSERT_TRUE(HasScope());
    const auto scope = RemoveScope();
    ASSERT_TRUE(scope);
    ASSERT_FALSE(HasScope());
    delete scope;
  }
};

TEST_F(LocalScopeTest, Test_New_NoParent) {  // NOLINT
  const auto scope = LocalScope::New();
  ASSERT_TRUE(scope);
  ASSERT_TRUE(scope->IsEmpty());
  ASSERT_TRUE(scope->IsRoot());
}

TEST_F(LocalScopeTest, Test_IsRoot) {  // NOLINT
  ASSERT_FALSE(GetScope()->GetParent());
  ASSERT_FALSE(GetScope()->HasParent());
  ASSERT_TRUE(GetScope()->IsRoot());

  const auto new_scope = LocalScope::New(GetScope());
  ASSERT_TRUE(new_scope);
  ASSERT_TRUE(new_scope->GetParent());
  ASSERT_TRUE(new_scope->HasParent());
  ASSERT_FALSE(new_scope->IsRoot());

  ASSERT_FALSE(GetScope()->GetParent());
  ASSERT_FALSE(GetScope()->HasParent());
  ASSERT_TRUE(GetScope()->IsRoot());
}

static constexpr const auto kSymbol1 = "sym1";
static constexpr const auto kSymbol2 = "sym2";
static constexpr const auto kSymbol3 = "sym3";

TEST_F(LocalScopeTest, Test_Has_Fails_NoSymbol) {  // NOLINT
  ASSERT_FALSE(GetScope()->Has(kSymbol1));
  ASSERT_FALSE(GetScope()->Has(kSymbol2));
  ASSERT_FALSE(GetScope()->Has(kSymbol3));
}

TEST_F(LocalScopeTest, Test_Has_Passes) {  // NOLINT
  ASSERT_FALSE(GetScope()->Has(kSymbol1));
  ASSERT_FALSE(GetScope()->Has(kSymbol2));
  ASSERT_FALSE(GetScope()->Has(kSymbol3));

  ASSERT_TRUE(GetScope()->Add(kSymbol2));

  ASSERT_FALSE(GetScope()->Has(kSymbol1));
  ASSERT_TRUE(GetScope()->Has(kSymbol2));
  ASSERT_FALSE(GetScope()->Has(kSymbol3));
}

TEST_F(LocalScopeTest, Test_ChildHasFromParentScope) {  // NOLINT
  ASSERT_FALSE(GetScope()->Has(kSymbol1));
  ASSERT_FALSE(GetScope()->Has(kSymbol2));
  ASSERT_FALSE(GetScope()->Has(kSymbol3));

  ASSERT_TRUE(GetScope()->Add(kSymbol2));

  ASSERT_FALSE(GetScope()->Has(kSymbol1));
  ASSERT_TRUE(GetScope()->Has(kSymbol2));
  ASSERT_FALSE(GetScope()->Has(kSymbol3));

  const auto child_scope = LocalScope::New(GetScope());
  ASSERT_TRUE(child_scope);

  ASSERT_FALSE(child_scope->Has(kSymbol1));
  ASSERT_FALSE(child_scope->Has(kSymbol2));
  ASSERT_FALSE(child_scope->Has(kSymbol3));
  ASSERT_TRUE(child_scope->Has(kSymbol2, true));
}
}  // namespace gel