#include <benchmark/benchmark.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "gel.h"
#include "heap.h"
#include "parser.h"
#include "runtime.h"

using namespace gel;

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);
  ::benchmark::Initialize(&argc, argv);
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  Heap::Init();
  Parser::Init();
  Runtime::Init();
  ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();
  return EXIT_SUCCESS;
}
