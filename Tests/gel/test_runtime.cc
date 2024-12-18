#include <glog/logging.h>
#include <gtest/gtest.h>

#include "gel/common.h"
#include "gel/runtime.h"
#include "gel/type_assertions.h"
#include "gtest/gtest.h"

namespace gel {
using namespace ::testing;

class RuntimeTest : public Test {
  DEFINE_NON_COPYABLE_TYPE(RuntimeTest);

 private:
  Runtime* runtime_ = nullptr;

 protected:
  RuntimeTest() = default;

  auto GetRuntime() const -> Runtime* {
    return runtime_;
  }

  inline auto SetRuntime(Runtime* rhs) -> Runtime* {
    const auto old = GetRuntime();
    runtime_ = rhs;
    return old;
  }

 public:
  ~RuntimeTest() override = default;

  void SetUp() override {
    ASSERT_FALSE(runtime_);
    runtime_ = Runtime::New();
    ASSERT_TRUE(runtime_);
  }

  void TearDown() override {
    ASSERT_TRUE(runtime_);
    delete runtime_;
    runtime_ = nullptr;
    ASSERT_FALSE(runtime_);
  }
};
}  // namespace gel