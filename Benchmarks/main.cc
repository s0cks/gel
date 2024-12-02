#include <benchmark/benchmark.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "gel/gel.h"
#include "gel/heap.h"
#include "gel/runtime.h"

using namespace gel;

auto main(int argc, char** argv) -> int {
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  ::google::InitGoogleLogging(argv[0]);
  ::benchmark::Initialize(&argc, argv);
  Heap::Init();
  Runtime::Init();
  ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();
  return EXIT_SUCCESS;
}