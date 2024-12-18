#include "gel/flow_graph_dot.h"
#ifdef GEL_ENABLE_GV

#include <fmt/format.h>
#include <glog/logging.h>

#include <cmath>
#include <cstdint>
#include <sstream>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/flow_graph.h"
#include "gel/flow_graph_builder.h"
#include "gel/gv.h"
#include "gel/instruction.h"
#include "gel/native_procedure.h"
#include "gel/object.h"

namespace gel {
namespace dot {
auto EffectVisitor::VisitGraphEntryInstr(ir::GraphEntryInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitTargetEntryInstr(ir::TargetEntryInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitJoinEntryInstr(ir::JoinEntryInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitGotoInstr(ir::GotoInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);

  const auto target = instr->GetTarget();
  ASSERT(target);
  BlockVisitor for_target(GetOwner());
  if (!target->Accept(&for_target)) {
    LOG(ERROR) << "failed to visit goto target.";
    return false;
  }

  if (for_target.HasEntry()) {
    const auto edge_id = fmt::format("blk{}blk{}", GetCurrentBlock()->GetBlockId(), target->GetBlockId());
    const auto edge = NewEdge(node, for_target.GetEntry(), edge_id.c_str());
    ASSERT(edge);
  }
  return true;
}

auto EffectVisitor::VisitBranchInstr(ir::BranchInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);

  Node* join = nullptr;
  if (SeenBlock(instr->GetJoin())) {
    join = GetBlockNode(instr->GetJoin());
  } else {
    BlockVisitor for_join(GetOwner());
    if (!instr->GetJoin()->Accept(&for_join))
      return false;
    join = for_join.GetEntry();
  }
  ASSERT(join);
  SetExit(node);  // TODO: fix, the Append above for the Join sets the exit to itself and we want the BranchInstr in this case

  {
    // true
    const auto target = instr->GetTrueTarget();
    ASSERT(target);
    BlockVisitor for_true(GetOwner());
    if (!target->Accept(&for_true))
      return false;
    if (for_true.HasEntry()) {
      const auto edge_id = fmt::format("blk{}blk{}", GetCurrentBlock()->GetBlockId(), target->GetBlockId());
      const auto edge = NewEdge(GetExit(), for_true.GetEntry(), edge_id.c_str());
      ASSERT(edge);
      SetEdgeLabel(edge, "#t");
    }
    if (for_true.HasExit() && join) {
      const auto edge_id = fmt::format("blk{}blk{}", target->GetBlockId(), instr->GetJoin()->GetBlockId());
      const auto edge = NewEdge(for_true.GetExit(), join, edge_id.c_str());
      ASSERT(edge);
    }
  }

  {
    // false
    const auto target = instr->GetFalseTarget();
    BlockVisitor for_false(GetOwner());
    if (target && !target->Accept(&for_false))
      return false;
    if (for_false.HasEntry()) {
      const auto edge_id = fmt::format("blk{}blk{}", GetCurrentBlock()->GetBlockId(), target->GetBlockId());
      const auto edge = NewEdge(GetExit(), for_false.GetEntry(), edge_id.c_str());
      ASSERT(edge);
      SetEdgeLabel(edge, "#f");
    }
    if (for_false.HasExit()) {
      const auto edge_id = fmt::format("blk{}blk{}", target->GetBlockId(), instr->GetJoin()->GetBlockId());
      const auto edge = NewEdge(for_false.GetExit(), join, edge_id.c_str());
      ASSERT(edge);
    }

    if (!target && !instr->HasNext() && join) {
      const auto edge_id = fmt::format("blk{}blk{}", GetCurrentBlock()->GetBlockId(), instr->GetJoin()->GetBlockId());
      const auto edge = NewEdge(node, join, edge_id.c_str());
      ASSERT(edge);
    }
  }

  BlockVisitor for_join(GetOwner());
  if (!instr->GetJoin()->Accept(&for_join))
    return false;

  SetExit(node);
  return true;
}

auto EffectVisitor::VisitLoadLocalInstr(ir::LoadLocalInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitStoreVariableInstr(ir::StoreVariableInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitUnaryOpInstr(ir::UnaryOpInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitBinaryOpInstr(ir::BinaryOpInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  std::stringstream label;
  label << instr->GetName() << std::endl;
  label << "Op: " << instr->GetOp();
  SetNodeLabel(node, label);
  return true;
}

auto EffectVisitor::VisitEvalInstr(ir::EvalInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitCastInstr(ir::CastInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitInvokeInstr(ir::InvokeInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitInvokeDynamicInstr(ir::InvokeDynamicInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

static inline auto GetTargetNativeProcedure(ir::InvokeNativeInstr* instr) -> NativeProcedure* {
  ASSERT(instr->GetTarget() && instr->GetTarget()->IsConstantInstr());
  const auto target = instr->GetTarget()->AsConstantInstr();
  ASSERT(target && target->GetValue()->IsNativeProcedure());
  return target->GetValue()->AsNativeProcedure();
}

auto EffectVisitor::VisitInvokeNativeInstr(ir::InvokeNativeInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  std::stringstream label;
  label << instr->GetName() << std::endl;
  const auto target = GetTargetNativeProcedure(instr);
  ASSERT(target);
  label << "Procedure: " << target->GetSymbol()->Get();
  SetNodeLabel(node, label);
  return true;
}

auto EffectVisitor::VisitReturnInstr(ir::ReturnInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitThrowInstr(ir::ThrowInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  return true;
}

auto EffectVisitor::VisitInstanceOfInstr(ir::InstanceOfInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  std::stringstream label;
  label << instr->GetName() << std::endl;
  label << "Value := ";
  PrintValue(label, instr->GetType());
  SetNodeLabel(node, label);
  return true;
}

auto EffectVisitor::VisitConstantInstr(ir::ConstantInstr* instr) -> bool {
  ASSERT(instr);
  const auto node = Append(instr);
  ASSERT(node);
  std::stringstream label;
  label << instr->GetName() << std::endl;
  label << "Value := ";
  PrintValue(label, instr->GetValue());
  SetNodeLabel(node, label);
  return true;
}

auto BlockVisitor::VisitGraphEntryInstr(ir::GraphEntryInstr* instr) -> bool {
  ASSERT(instr);
  GetOwner()->SetBlock(instr);
  if (!EffectVisitor::VisitGraphEntryInstr(instr))
    return false;
  ASSERT(instr && instr->HasNext() && instr->GetNext()->IsTargetEntryInstr());
  const auto target = instr->GetNext()->AsTargetEntryInstr();

  BlockVisitor for_target_effect(GetOwner());
  if (!target->Accept(&for_target_effect))
    return false;

  if (for_target_effect.HasEntry()) {
    const auto edge_id = fmt::format("blk{}blk{}", instr->GetBlockId(), target->GetBlockId());
    const auto edge = NewEdge(GetExit(), for_target_effect.GetEntry(), edge_id.c_str());
    ASSERT(edge);
  }
  return true;
}

auto BlockVisitor::VisitTargetEntryInstr(ir::TargetEntryInstr* instr) -> bool {
  ASSERT(instr);
  if (SeenBlock(instr))
    return true;

  GetOwner()->SetBlock(instr);
  if (!EffectVisitor::VisitTargetEntryInstr(instr))
    return false;

  ir::InstructionIterator iter(instr->GetFirstInstruction());
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    if (!next->Accept(this))
      return false;
  }
  return true;
}

auto BlockVisitor::VisitJoinEntryInstr(ir::JoinEntryInstr* instr) -> bool {
  ASSERT(instr);
  if (SeenBlock(instr))
    return true;

  GetOwner()->SetBlock(instr);
  if (!EffectVisitor::VisitJoinEntryInstr(instr))
    return false;
  ir::InstructionIterator iter(instr->GetFirstInstruction());
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    if (!next->Accept(this))
      return false;
  }
  return true;
}
}  // namespace dot

auto FlowGraphToDotGraph::Build() -> dot::Graph* {
  const auto flow_graph = GetFlowGraph();
  ASSERT(flow_graph);
  SetNodeAttr("shape", "box");
  SetNodeAttr("label", "");
  SetNodeAttr("xlabel", "");
  SetEdgeAttr("label", "");
  SetNodeAttr("width", "1.5");
  const auto graph_entry = flow_graph->GetEntry();
  ASSERT(graph_entry);
  dot::BlockVisitor vis(this);
  LOG_IF(FATAL, !graph_entry->Accept(&vis)) << "failed to visit: " << graph_entry->ToString();
  return dot::Graph::New(this);
}
}  // namespace gel

#endif  // GEL_ENABLE_GV