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
  for (const auto& blk : flow_graph->GetPreorder()) {
    ASSERT(blk);
    DLOG(INFO) << "compiling block " << blk->ToString();
    ir::InstructionIterator iter(blk);
    while (iter.HasNext()) {
      const auto next = iter.Next();
      ASSERT(next);
      DLOG(INFO) << "compiling " << next->ToString();
      next->Compile(this);
    }
  }
}

template <class E>
auto FlowGraphCompiler::BuildFlowGraph(E* exec, std::enable_if_t<gel::is_executable<E>::value>*) -> FlowGraph* {
  TRACE_ZONE_NAMED("FlowGraphCompiler::BuildFlowGraph");
  ASSERT(exec);
  const auto scope = LocalScope::New(GetScope());
  if (exec->HasScope())
    scope->AddAll(exec->GetScope());
  const auto flow_graph = FlowGraphBuilder::Build(exec, scope);
  LOG_IF(FATAL, !(flow_graph && flow_graph->HasEntry())) << "failed to build FlowGraph for: " << exec;
  if (FLAGS_print_ir) {
    DLOG(INFO) << exec->ToString() << " flow graph:";
  }
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
template auto FlowGraphCompiler::CompileTarget(Constructor* script, void*) -> bool;

template <class E>
auto FlowGraphCompiler::CompileTarget(E* exec, std::enable_if_t<gel::is_executable<E>::value>*) -> bool {
  TRACE_ZONE_NAMED("FlowGraphCompiler::CompileTarget");
  ASSERT(exec);
  TIMER_START;
  MacroExpander::ExpandAll(exec, GetScope());
  auto flow_graph = BuildFlowGraph(exec);
  ASSERT(flow_graph && flow_graph->HasEntry());

  flow_graph->DiscoverBlocks();
  flow_graph->ComputeSSA(0);

  AssembleFlowGraph(flow_graph);
  TIMER_STOP(total_ns);
  CompiledCode code(assembler_.Assemble());
  if (!code.IsCompiled()) {
    LOG(ERROR) << "failed to compile: " << exec;
    return false;
  }
  DVLOG(10) << "compiled in " << units::time::nanosecond_t(static_cast<double>(total_ns));
  code.SetCompileTime(total_ns);
  exec->SetCode(code);
  if (VLOG_IS_ON(10) || FLAGS_print_bytecode)
    Disassembler::Disassemble(std::cout, exec, GetScope());
  TRACE_TAG_STR(exec->GetFullyQualifiedName());
  TRACE_MARK;
  return true;
}
}  // namespace gel