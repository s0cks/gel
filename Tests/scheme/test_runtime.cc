#include <glog/logging.h>
#include <gtest/gtest.h>

#include "gtest/gtest.h"
#include "scheme/common.h"
#include "scheme/runtime.h"
#include "type_assertions.h"

namespace scm {
using namespace ::testing;

class RuntimeTest : public Test {
  DEFINE_NON_COPYABLE_TYPE(RuntimeTest);

 private:
  Runtime* runtime_ = nullptr;
  Environment* globals_ = nullptr;

 protected:
  RuntimeTest() = default;

  auto GetRuntime() const -> Runtime* {
    return runtime_;
  }

  auto GetGlobals() const -> Environment* {
    return globals_;
  }

  inline auto SetGlobals(Environment* rhs) -> Environment* {
    const auto old = GetGlobals();
    globals_ = rhs;
    return old;
  }

  inline auto SetRuntime(Runtime* rhs) -> Runtime* {
    const auto old = GetRuntime();
    runtime_ = rhs;
    return old;
  }

 public:
  ~RuntimeTest() override = default;

  void SetUp() override {
    globals_ = Environment::New();
    ASSERT_TRUE(globals_);
    runtime_ = Runtime::New(globals_);
    ASSERT_TRUE(runtime_);
  }

  void TearDown() override {
    {
      const auto old = SetRuntime(nullptr);
      delete old;
    }

    {
      const auto old = SetGlobals(nullptr);
      delete old;
    }
  }
};

TEST_F(RuntimeTest, Test_Eval_Fails_NullExpression) {  // NOLINT
  ASSERT_DEATH(GetRuntime()->Eval(nullptr), "");       // NOLINT
}

TEST_F(RuntimeTest, Test_Eval_Passes_Datum) {  // NOLINT
  static const auto kExpression = Bool::True();
  const auto result = GetRuntime()->Eval(kExpression);
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->IsBool());
  ASSERT_TRUE(result->AsBool()->Get());
  DLOG(INFO) << "result: " << result->ToString();
}

TEST_F(RuntimeTest, Test_Eval_Passes_Symbol) {  // NOLINT
  ASSERT_TRUE(GetGlobals()->Put("test", Bool::True()));
  {
    const auto result = GetRuntime()->Eval(Symbol::New("missing"));
    ASSERT_TRUE(IsNull(result));
  }
  {
    const auto result = GetRuntime()->Eval(Symbol::New("test"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->IsBool());
    ASSERT_TRUE(result->AsBool()->Get());
    DLOG(INFO) << "test := " << result->ToString();
  }
}

TEST_F(RuntimeTest, Test_Eval_BinaryOp_Add_Success) {  // NOLINT
  static const auto kExpression = Pair::New(Symbol::New("+"), Pair::New(Number::New(99), Pair::New(Number::New(1), Null::Get())));
  ASSERT_TRUE(kExpression);
  const auto result = GetRuntime()->Eval(kExpression);
  ASSERT_TRUE(result);
  DLOG(INFO) << "result: " << result;
  ASSERT_TRUE(result->IsNumber());
  ASSERT_EQ(result->AsNumber()->GetValue(), 100);
}

TEST_F(RuntimeTest, Test_Eval_BinaryOp_Subtract_Success) {  // NOLINT
  static const auto kExpression = Pair::New(
      Symbol::New("-"), Pair::New(Number::New(100), Pair::New(Number::New(50), Pair::New(Number::New(25), Null::Get()))));
  ASSERT_TRUE(kExpression);
  const auto result = GetRuntime()->Eval(kExpression);
  ASSERT_TRUE(result);
  DLOG(INFO) << "result: " << result;
  ASSERT_TRUE(result->IsNumber());
  ASSERT_EQ(result->AsNumber()->GetValue(), 25);
}
}  // namespace scm