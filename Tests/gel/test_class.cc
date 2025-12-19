#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_pointer_visitor.h"
#include "types.h"
#include "gmock/gmock.h"

namespace gel {
class ClassTest : public ::testing::Test {
  DEFINE_NON_COPYABLE_TYPE(ClassTest);

 protected:
  ClassTest() = default;

 public:
  ~ClassTest() override = default;
};

MATCHER_P(PointerEqClass, rhs, "") {  // NOLINT
  DLOG(INFO) << "arg: " << arg->GetObjectPointer()->ToString() << " == " << rhs->ToString();
  return arg && arg->Equals(rhs->raw_ptr());
}

TEST_F(ClassTest, Test_VisitAllClassPointers) {
  Class::PrintAllClasses();
  MockPointerVisitor vis;
  ON_CALL(vis, Visit(::testing::_)).WillByDefault(::testing::Return(false));
#define EXPECT_VISIT_TYPE_CLASS(Name) EXPECT_CALL(vis, Visit(PointerEqClass(Name::GetClass()))).WillOnce(::testing::Return(true));
  FOR_EACH_TYPE(EXPECT_VISIT_TYPE_CLASS)
  EXPECT_VISIT_TYPE_CLASS(Object);
  EXPECT_VISIT_TYPE_CLASS(ArrayBase);
#undef EXPECT_VISIT_TYPE_CLASS
  // TODO: ASSERT_TRUE(Class::VisitAllClassPointers(&vis));
}
}  // namespace gel
