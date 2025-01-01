#include "gel/flow_graph_compiler.h"

#include <chrono>
#include <sstream>
#include <type_traits>

#include "gel/assembler.h"
#include "gel/common.h"
#include "gel/disassembler.h"
#include "gel/flow_graph_builder.h"
#include "gel/instruction.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/macro_expander.h"
#include "gel/script.h"
#include "gel/tracing.h"

namespace gel {
void FlowGraphCompiler::AssembleFlowGraph(FlowGraph* flow_graph) {
  TRACE_ZONE_NAMED("FlowGraphCompiler::AssembleFlowGraph");
  ASSERT(flow_graph && flow_graph->HasEntry());
  ir::InstructionIterator iter(flow_graph->GetEntry());
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    next->Compile(this);
  }
}

template <class E>
auto FlowGraphCompiler::BuildFlowGraph(E* exec, std::enable_if_t<gel::is_executable<E>::value>*) -> FlowGraph* {
  TRACE_ZONE_NAMED("FlowGraphCompiler::BuildFlowGraph");
  ASSERT(exec);
  const auto scope = LocalScope::New(GetScope());
  if (exec->HasScope())
    LOG_IF(ERROR, !scope->Add(exec->GetScope())) << "failed to add " << exec << " scope to current scope.";
  FlowGraphBuilder builder(scope);
  const auto flow_graph = builder.Build(exec, scope);
  LOG_IF(FATAL, !(flow_graph && flow_graph->HasEntry())) << "failed to build FlowGraph for: " << exec;
  return flow_graph;
}

auto FlowGraphCompiler::GetBlockInfo(ir::EntryInstr* blk) -> BlockInfo& {
  ASSERT(blk);
  return GetBlockInfo(blk->GetBlockId());
}

auto FlowGraphCompiler::GetBlockLabel(ir::EntryInstr* blk) -> Label* {
  ASSERT(blk);
  return GetBlockLabel(blk->GetBlockId());
}

template auto FlowGraphCompiler::CompileTarget(Lambda* lambda, void*) -> bool;
template auto FlowGraphCompiler::CompileTarget(Script* script, void*) -> bool;

template <class E>
auto FlowGraphCompiler::CompileTarget(E* exec, std::enable_if_t<gel::is_executable<E>::value>*) -> bool {
  TRACE_ZONE_NAMED("FlowGraphCompiler::CompileTarget");
  ASSERT(exec);
  if (exec->IsEmpty()) {
    DLOG(ERROR) << "cannot compile: " << exec;
    return false;
  }
  TIMER_START;
  MacroExpander::ExpandAll(exec, GetScope());
  const auto flow_graph = BuildFlowGraph(exec);
  ASSERT(flow_graph && flow_graph->HasEntry());
  AssembleFlowGraph(flow_graph);
  TIMER_STOP(total_ns);
  const auto code = assembler_.Assemble();
  exec->SetCodeRegion(code);
#ifdef GEL_DEBUG
  DVLOG(10) << exec << " compiled in " << units::time::nanosecond_t(static_cast<double>(total_ns));
  exec->SetCompileTime(total_ns);
  if (VLOG_IS_ON(10))
    Disassembler::Disassemble(std::cout, exec, GetScope());
#endif  // GEL_DEBUG
  TRACE_TAG_STR(exec->GetFullyQualifiedName());
  TRACE_MARK;
  return exec->IsCompiled();
}
}  // namespace gel