#include <glog/logging.h>
#include <gtest/gtest.h>

#include "scheme/scheme.h"
#include "scheme/type.h"

using namespace scm;

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  ::testing::InitGoogleTest(&argc, argv);
  ::google::ParseCommandLineFlags(&argc, &argv, false);
  LOG(INFO) << "Running unit tests for scheme v" << scm::GetVersion() << "....";
  Type::Init();
  return RUN_ALL_TESTS();
}