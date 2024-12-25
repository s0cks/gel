#include <glog/logging.h>
#include <gtest/gtest.h>

#include "gel/gel.h"
#include "gel/heap.h"
#include "gel/object.h"

using namespace gel;

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  ::google::ParseCommandLineFlags(&argc, &argv, false);
  LOG(INFO) << "Running unit tests for scheme v" << gel::GetVersion() << "....";
  Heap::Init();
  Object::Init();
  return RUN_ALL_TESTS();
}