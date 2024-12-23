#include "gel/flow_graph_compiler.h"

#include <chrono>
#include <sstream>

#include "gel/assembler.h"
#include "gel/common.h"
#include "gel/disassembler.h"
#include "gel/flow_graph_builder.h"
#include "gel/instruction.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/script.h"
#include "gel/tracing.h"

namespace gel {
void FlowGraphCompiler::AssembleFlowGraph(FlowGraph* flow_graph) {
  TRACE_ZONE_NAMED("AssembleFlowGraph");
  ASSERT(flow_graph && flow_graph->HasEntry());
  ir::InstructionIterator iter(flow_graph->GetEntry());
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    next->Compile(this);
  }
}

auto FlowGraphCompiler::BuildFlowGraph(Lambda* lambda) -> FlowGraph* {
  TRACE_ZONE_NAMED("BuildFlowGraph");
  ASSERT(lambda);
  FlowGraphBuilder builder(GetScope());
  const auto flow_graph = builder.Build(lambda, GetScope());
  LOG_IF(FATAL, !(flow_graph && flow_graph->HasEntry())) << "failed to build FlowGraph for: " << lambda;
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

auto FlowGraphCompiler::BuildFlowGraph(Script* script) -> FlowGraph* {
  TRACE_ZONE_NAMED("BuildFlowGraph");
  ASSERT(script);
  const auto scope = LocalScope::Union(
      {
          script->GetScope(),
      },
      GetScope());
  FlowGraphBuilder builder(scope);
  const auto flow_graph = builder.Build(script, scope);
  LOG_IF(FATAL, !(flow_graph && flow_graph->HasEntry())) << "failed to build FlowGraph for: " << script;
  return flow_graph;
}

auto FlowGraphCompiler::CompileLambda(Lambda* lambda) -> bool {
  TRACE_MARK;
  TRACE_ZONE_NAMED("CompileLambda");
  ASSERT(lambda);
  TIMER_START;
  const auto flow_graph = BuildFlowGraph(lambda);
  ASSERT(flow_graph && flow_graph->HasEntry());
  AssembleFlowGraph(flow_graph);
  TIMER_STOP(total_ns);
  const auto code = assembler_.Assemble();
  lambda->SetCodeRegion(code);
#ifdef GEL_DEBUG
  DVLOG(1000) << lambda << " compiled in " << units::time::nanosecond_t(static_cast<double>(total_ns));
  lambda->SetCompileTime(total_ns);
  if (VLOG_IS_ON(10)) {
    const auto scope = LocalScope::New(GetScope());
    const auto self_local = LocalVariable::New(scope, lambda->HasSymbol() ? lambda->GetSymbol() : Symbol::New("$"), lambda);
    ASSERT(self_local);
    LOG_IF(FATAL, !scope->Add(self_local)) << "failed to add " << (*self_local) << " to scope.";
    for (const auto& arg : lambda->GetArgs()) {
      const auto local = LocalVariable::New(scope, arg.GetName());
      ASSERT(local);
      LOG_IF(FATAL, !scope->Add(local)) << "failed to add " << local << " to scope.";
    }
    const auto label = lambda->HasSymbol() ? lambda->GetSymbol()->Get().c_str() : "lambda";
    std::stringstream ss;
    Disassembler disassembler(ss, scope);
    disassembler.Disassemble(code, label);
    std::cout << ss.str();
  }
#endif  // GEL_DEBUG
  return lambda;
}

auto FlowGraphCompiler::CompileScript(Script* script) -> bool {
  TRACE_MARK;
  TRACE_ZONE_NAMED("CompileScript");
  ASSERT(script);
  TIMER_START;
  const auto flow_graph = BuildFlowGraph(script);
  ASSERT(flow_graph && flow_graph->HasEntry());
  AssembleFlowGraph(flow_graph);
  TIMER_STOP(total_ns);
  const auto code = assembler_.Assemble();
  script->SetCodeRegion(code);
#ifdef GEL_DEBUG
  DVLOG(1000) << script << " compiled in " << units::time::nanosecond_t(static_cast<double>(total_ns));
  script->SetCompileTime(total_ns);
  if (VLOG_IS_ON(10)) {
    const auto scope = LocalScope::New(GetScope());
    std::stringstream ss;
    Disassembler disassembler(ss, scope);
    disassembler.Disassemble(code, "Script");
    std::cout << ss.str();
  }
#endif  // GEL_DEBUG
  return script;
}

auto FlowGraphCompiler::Compile(Lambda* lambda, LocalScope* scope) -> bool {
  ASSERT(lambda);
  if (lambda->IsCompiled())
    return lambda;
  ASSERT(scope);
  FlowGraphCompiler compiler(scope);
  return compiler.CompileLambda(lambda);
}

auto FlowGraphCompiler::Compile(Script* script, LocalScope* scope) -> bool {
  ASSERT(script);
  if (script->IsCompiled())
    return true;
  ASSERT(scope);
  FlowGraphCompiler compiler(scope);
  return compiler.CompileScript(script);
}
}  // namespace gel