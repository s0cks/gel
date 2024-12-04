#include "gel/flow_graph_compiler.h"

#include <chrono>

#include "gel/flow_graph_builder.h"

namespace gel {
auto FlowGraphCompiler::CompileLambda(Lambda* lambda) -> bool {
  ASSERT(lambda);
#ifdef GEL_DEBUG
  const auto start_ns = Clock::now();
#endif  // GEL_DEBUG
  FlowGraphBuilder builder(GetScope());
  const auto flow_graph = builder.Build(lambda, GetScope());
  if (!flow_graph || !flow_graph->HasEntry()) {
    LOG(ERROR) << "failed to build FlowGraph for: " << lambda->ToString();
    return false;
  }
  lambda->SetEntry(flow_graph->GetEntry());
#ifdef GEL_DEBUG
  const auto stop_ns = Clock::now();
  const auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop_ns - start_ns).count();
  DVLOG(1000) << lambda << " compiled in " << units::time::nanosecond_t(static_cast<double>(total_ns));
  lambda->SetCompileTime(total_ns);
#endif  // GEL_DEBUG
  return lambda;
}

auto FlowGraphCompiler::Compile(Lambda* lambda, LocalScope* scope) -> bool {
  ASSERT(lambda);
  if (lambda->IsCompiled())
    return lambda;
  ASSERT(scope);
  FlowGraphCompiler compiler(scope);
  return compiler.CompileLambda(lambda);
}
}  // namespace gel