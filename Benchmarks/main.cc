#include <benchmark/benchmark.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "gel/gel.h"

auto main(int argc, char** argv) -> int {
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  ::google::InitGoogleLogging(argv[0]);
  ::benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();
  return EXIT_SUCCESS;
}