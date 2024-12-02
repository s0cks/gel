#include <benchmark/benchmark.h>
#include <fmt/format.h>

#include "gel/common.h"
#include "gel/runtime.h"

namespace gel {
void BM_Factorial_Execution(benchmark::State& state) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto expr = fmt::format("(factorial {0:d})", state.range(0));
  DLOG(INFO) << "evaluating: " << expr;
  for (const auto& _ : state) {
    const auto start = Clock::now();
    const auto value = runtime->Eval(expr);
    const auto end = Clock::now();
    state.SetIterationTime(std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count());
  }
}

BENCHMARK(BM_Factorial_Execution)->Arg(7)->Arg(10);

}  // namespace gel