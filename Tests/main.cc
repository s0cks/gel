#include <glog/logging.h>
#include <gtest/gtest.h>

#include "gel.h"
#include "heap.h"
#include "object.h"
#include "parser.h"
#include "runtime.h"

using namespace gel;

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  ::google::ParseCommandLineFlags(&argc, &argv, false);
  LOG(INFO) << "Running unit tests for scheme v" << gel::GetVersion() << "....";
  Parser::Init();
  Heap::Init();
  Runtime::Init();
  return RUN_ALL_TESTS();
}
