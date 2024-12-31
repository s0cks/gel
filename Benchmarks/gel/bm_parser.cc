#include <benchmark/benchmark.h>
#include <fmt/format.h>

#include "gel/common.h"
#include "gel/parser.h"

namespace gel {
void BM_Parser_Parse_InvokeClosure(benchmark::State& state) {
  const auto expr = "((fn test/get-string-cid [] \"Returns the ClassId of String\" (gel/get-class-id 'String)))";
  DLOG(INFO) << "evaluating: " << expr;
  for (const auto& _ : state) {
    const auto start = Clock::now();
    const auto parsed_expr = Parser::ParseExpr(expr);
    const auto end = Clock::now();
    ASSERT(parsed_expr);
    state.SetIterationTime(std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count());
  }
}

BENCHMARK(BM_Parser_Parse_InvokeClosure)->Iterations(20)->UseManualTime();  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
}  // namespace gel