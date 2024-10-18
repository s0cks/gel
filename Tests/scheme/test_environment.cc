#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "scheme/common.h"
#include "scheme/environment.h"

namespace scm {
using namespace ::testing;

class EnvironmentTest : public Test {
  DEFINE_NON_COPYABLE_TYPE(EnvironmentTest);

 private:
  Environment* env_ = nullptr;

  auto SetEnv(Environment* new_env) -> Environment* {
    ASSERT(new_env);
    const auto old_env = GetEnv();
    env_ = new_env;
    return old_env;
  }

 protected:
  EnvironmentTest() = default;

 public:
  ~EnvironmentTest() override = default;

  inline auto GetEnv() const -> Environment* {
    return env_;
  }

  void SetUp() override {
    const auto new_env = Environment::New();
    ASSERT_EQ(SetEnv(new_env), nullptr);
  }

  void TearDown() override {
    const auto old_env = SetEnv(nullptr);
    ASSERT_NE(old_env, nullptr);
    delete old_env;
  }
};

TEST_F(EnvironmentTest, Test_New) {  // NOLINT
  const auto env = Environment::New();
  ASSERT_TRUE(env);
}

TEST_F(EnvironmentTest, Test_GetTotalNumberOfLocalValues) {  // NOLINT
  ASSERT_EQ(GetEnv()->GetTotalNumberOfLocalValues(), 0);
  ASSERT_TRUE(GetEnv()->Put("test", Bool::True()));
  ASSERT_EQ(GetEnv()->GetTotalNumberOfLocalValues(), 1);
}

TEST_F(EnvironmentTest, Test_GetTotalNumberOfValues) {  // NOLINT
  ASSERT_TRUE(GetEnv()->IsEmpty());
  ASSERT_EQ(GetEnv()->GetTotalNumberOfValues(), 0);
  ASSERT_TRUE(GetEnv()->Put("test", Bool::True()));
  ASSERT_FALSE(GetEnv()->IsEmpty());
  ASSERT_EQ(GetEnv()->GetTotalNumberOfValues(), 1);
  const auto env = Environment::New(GetEnv());
  ASSERT_TRUE(env);
  ASSERT_TRUE(env->IsEmpty());
  ASSERT_EQ(env->GetTotalNumberOfLocalValues(), 0);
  ASSERT_EQ(env->GetTotalNumberOfValues(), 1);
}

TEST_F(EnvironmentTest, Test_Put) {  // NOLINT
  ASSERT_TRUE(GetEnv()->Put("test", Bool::True()));
  ASSERT_FALSE(GetEnv()->Put("test", Bool::False()));
}

TEST_F(EnvironmentTest, Test_Has_Local) {  // NOLINT
  ASSERT_TRUE(GetEnv()->IsEmpty());
  ASSERT_TRUE(GetEnv()->Put("test", Bool::True()));
  ASSERT_FALSE(GetEnv()->IsEmpty());
  ASSERT_EQ(GetEnv()->GetTotalNumberOfLocalValues(), 1);
  ASSERT_TRUE(GetEnv()->Has("test"));
}

TEST_F(EnvironmentTest, Test_Has_Parent) {  // NOLINT
  ASSERT_TRUE(GetEnv()->IsEmpty());
  ASSERT_TRUE(GetEnv()->Put("test", Bool::True()));
  ASSERT_FALSE(GetEnv()->IsEmpty());
  ASSERT_EQ(GetEnv()->GetTotalNumberOfLocalValues(), 1);
  const auto env = Environment::New(GetEnv());
  ASSERT_TRUE(env);
  ASSERT_TRUE(env->IsEmpty());
  ASSERT_EQ(env->GetTotalNumberOfLocalValues(), 0);
  ASSERT_EQ(env->GetTotalNumberOfValues(), 1);
  ASSERT_TRUE(env->Has("test"));
}

TEST_F(EnvironmentTest, Test_IsEmpty) {  // NOLINT
  ASSERT_TRUE(GetEnv()->IsEmpty());
  ASSERT_TRUE(GetEnv()->Put("test", Bool::True()));
  ASSERT_FALSE(GetEnv()->IsEmpty());
}
}  // namespace scm