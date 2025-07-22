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
    DVLOG(10) << "compiling " << blk->ToString();
    ir::InstructionIterator iter(blk);
    while (iter.HasNext()) {
      const auto next = iter.Next();
      ASSERT(next);
      DVLOG(10) << "compiling " << next->ToString() << "....";
      next->Compile(this);
    }
  }
}

template <CompilerTarget Target>
auto FlowGraphCompiler::BuildFlowGraph(Target& target) -> FlowGraph* {
  TRACE_ZONE_NAMED("FlowGraphCompiler::BuildFlowGraph");
  const auto flow_graph = FlowGraphBuilder::Build(target);
  LOG_IF(FATAL, !(flow_graph && flow_graph->HasEntry())) << "failed to build FlowGraph for: " << target;
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

template auto FlowGraphCompiler::CompileTarget(LambdaFn& lambda) -> bool;
template auto FlowGraphCompiler::CompileTarget(Script& script) -> bool;
template auto FlowGraphCompiler::CompileTarget(InitFn& script) -> bool;

template <CompilerTarget Target>
auto FlowGraphCompiler::CompileTarget(Target& target) -> bool {
  TRACE_ZONE_NAMED("FlowGraphCompiler::CompileTarget");
  TIMER_START;
  MacroExpander::ExpandAll(&target, GetScope());
  auto flow_graph = BuildFlowGraph(target);
  ASSERT(flow_graph && flow_graph->HasEntry());

  flow_graph->DiscoverBlocks();
  flow_graph->ComputeSSA(16);

  AssembleFlowGraph(flow_graph);
  TIMER_STOP(total_ns);
  const auto code = CompiledCode::New(assembler_.Assemble());
  if (!code->IsCompiled()) {
    LOG(ERROR) << "failed to compile: " << target;
    return false;
  }
  code->SetCompileTime(total_ns);
  target.SetCode(code);
  DVLOG(10) << (*code) << " compiled in " << units::time::nanosecond_t(static_cast<double>(total_ns));
  if (VLOG_IS_ON(1) || FLAGS_print_bytecode)
    Disassembler::Disassemble(std::cout, target, GetScope());
  TRACE_TAG_STR(target.GetFullyQualifiedName());
  TRACE_MARK;
  return true;
}
}  // namespace gel