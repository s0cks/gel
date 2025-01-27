#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "gel/array.h"
#include "gel/common.h"
#include "gel/mock_pointer_visitor.h"

namespace gel {
class ArrayTest : public ::testing::Test {
  DEFINE_NON_COPYABLE_TYPE(ArrayTest);

 protected:
  ArrayTest() = default;

 public:
  ~ArrayTest() override = default;
};

MATCHER_P(IsConstLong, rhs, "") {  // NOLINT
  return arg && (arg->IsLong() && arg->AsLong()->Get() == rhs);
}

MATCHER_P(PtrEqPtr, rhs, "") {  // NOLINT
  ASSERT(rhs);
  return arg && rhs && (arg->GetStartingAddress() == rhs->GetStartingAddress() && arg->GetTotalSize() == rhs->GetTotalSize());
}

TEST_F(ArrayTest, Test_Array) {
  Array<Long*> array;
  ASSERT_NO_FATAL_FAILURE(array.Set(0, Long::New(10)));
  ASSERT_NO_FATAL_FAILURE(array.Set(1, Long::New(11)));
  ASSERT_NO_FATAL_FAILURE(array.Set(2, Long::New(12)));
  ASSERT_THAT(array.Get(0), IsConstLong(10));
  ASSERT_THAT(array.Get(1), IsConstLong(11));
  ASSERT_THAT(array.Get(2), IsConstLong(12));
}

MATCHER_P(IsPtrPtrTo, rhs, "") {  // NOLINT
  ASSERT(rhs);
  return arg && testing::ExplainMatchResult(PtrEqPtr(rhs->raw_ptr()), (*arg), result_listener);
}

TEST_F(ArrayTest, Test_VisitPoineters) {
  const auto a = Long::New(10);
  ASSERT_TRUE(a);
  const auto b = Long::New(12);
  ASSERT_TRUE(b);
  const auto c = Long::New(14);
  ASSERT_TRUE(c);

  Array<Long*> array;
  ASSERT_NO_FATAL_FAILURE(array.Set(0, a));
  ASSERT_NO_FATAL_FAILURE(array.Set(1, b));
  ASSERT_NO_FATAL_FAILURE(array.Set(2, c));

  MockPointerPointerVisitor vis;
  ON_CALL(vis, Visit(::testing::_)).WillByDefault([](Pointer** ptr) {
    LOG(INFO) << "visiting: " << *(*ptr);
    return true;
  });
  EXPECT_CALL(vis, Visit(IsPtrPtrTo(a)));
  EXPECT_CALL(vis, Visit(IsPtrPtrTo(b)));
  EXPECT_CALL(vis, Visit(IsPtrPtrTo(c)));

  ASSERT_TRUE(array.VisitPointerPointers(&vis));
}
}  // namespace gel