#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "gel/common.h"

namespace gel {
using namespace ::testing;

class InstructionTest : public Test {
  DEFINE_NON_COPYABLE_TYPE(InstructionTest);

 public:
  InstructionTest() = default;
  ~InstructionTest() override = default;
};

}  // namespace gel