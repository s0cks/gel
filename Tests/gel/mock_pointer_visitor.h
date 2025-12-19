#ifndef GEL_MOCK_POINTER_VISITOR_H
#define GEL_MOCK_POINTER_VISITOR_H

#include <gmock/gmock.h>

#include "common.h"
#include "pointer.h"

namespace gel {
class MockPointerVisitor : public PointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(MockPointerVisitor);

 public:
  MockPointerVisitor() :
    PointerVisitor() {
    ON_CALL(*this, Visit(::testing::_)).WillByDefault(::testing::Return(true));
  }
  ~MockPointerVisitor() override = default;
  MOCK_METHOD(bool, Visit, (Pointer*));  // NOLINT
};

class MockPointerPointerVisitor : public PointerPointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(MockPointerPointerVisitor);

 public:
  MockPointerPointerVisitor() :
    PointerPointerVisitor() {
    ON_CALL(*this, Visit(::testing::_)).WillByDefault(::testing::Return(true));
  }
  ~MockPointerPointerVisitor() override = default;
  MOCK_METHOD(bool, Visit, (Pointer**));  // NOLINT
};
}  // namespace gel

#endif  // GEL_MOCK_POINTER_VISITOR_H
