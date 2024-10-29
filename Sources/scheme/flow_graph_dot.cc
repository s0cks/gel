#include "scheme/flow_graph_dot.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include <cmath>
#include <cstdint>
#include <sstream>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/flow_graph.h"
#include "scheme/gv.h"
#include "scheme/instruction.h"

namespace scm {
class DotVisitor : public InstructionVisitor, dot::GraphDecorator {
  using InstrNode = std::pair<Instruction*, Node*>;
  DEFINE_NON_COPYABLE_TYPE(DotVisitor);

 private:
  FlowGraphToDotGraph* owner_ = nullptr;
  NodeList nodes_{};
  Node* entry_node_ = nullptr;
  Node* exit_node_ = nullptr;
  EntryInstr* block_ = nullptr;

  inline void SetBlock(EntryInstr* blk) {
    ASSERT(blk);
    block_ = blk;
  }

  inline auto GetBlock() const -> EntryInstr* {
    return block_;
  }

  inline auto HasBlock() const -> bool {
    return GetBlock() != nullptr;
  }

  inline void SetEntryNode(Node* node) {
    ASSERT(node);
    entry_node_ = node;
  }

  inline auto GetEntryNode() const -> Node* {
    return entry_node_;
  }

  inline auto HasEntryNode() const -> bool {
    return GetEntryNode() != nullptr;
  }

  inline void SetExitNode(Node* node) {
    ASSERT(node);
    exit_node_ = node;
  }

  inline auto GetExitNode() const -> Node* {
    return exit_node_;
  }

  inline auto HasExitNode() const -> bool {
    return GetExitNode() != nullptr;
  }

  inline void SetOwner(FlowGraphToDotGraph* owner) {
    ASSERT(owner);
    owner_ = owner;
  }

  inline auto CreateNode() -> Node* {
    const auto node_id = fmt::format("b{0:d}n{1:d}", GetBlock()->GetBlockId(), nodes_.size() + 1);
    const auto node = NewNode(node_id);
    ASSERT(node);
    nodes_.push_back(node);
    return node;
  }

  inline auto CreateNode(Instruction* instr) -> Node* {
    ASSERT(instr);
    const auto node = CreateNode();
    SetNodeLabel(node, instr->GetName());
    return node;
  }

  inline void Append(Node* node, const bool create_edge = true) {
    ASSERT(node);
    if (!HasEntryNode()) {
      SetEntryNode(node);
      SetExitNode(node);
      return;
    }

    if (HasExitNode() && create_edge) {
      const auto previous = GetExitNode();
      const auto edge = NewEdge(previous, node, "");
      ASSERT(edge);
    }
    SetExitNode(node);
  }

  inline auto Append(Instruction* instr, const bool create_edge = true) -> Node* {
    ASSERT(instr);
    const auto node = CreateNode(instr);
    ASSERT(node);
    Append(node, create_edge);
    return node;
  }

 public:
  explicit DotVisitor(FlowGraphToDotGraph* owner, Agraph_t* graph) :
    InstructionVisitor(),
    dot::GraphDecorator(graph) {
    SetOwner(owner);
  }
  ~DotVisitor() override = default;

  auto GetOwner() const -> FlowGraphToDotGraph* {
    return owner_;
  }

  auto VisitGraphEntryInstr(GraphEntryInstr* instr) -> bool override {
    ASSERT(instr);
    SetBlock(instr);
    Append(instr);
    if (!instr->HasNext())
      return true;
    const auto next = instr->GetNext();
    ASSERT(next);

    DotVisitor vis(GetOwner(), GetGraph());
    if (!next->Accept(&vis))
      return false;

    if (HasExitNode() && vis.HasEntryNode()) {
      const auto edge_id = fmt::format("blk{0:d}blk{1:d}", GetBlock()->GetBlockId(), vis.GetBlock()->GetBlockId());
      const auto edge = NewEdge(GetExitNode(), vis.GetEntryNode(), edge_id.c_str());
      ASSERT(edge);
    }

    SetExitNode(vis.GetExitNode());
    return true;
  }

  auto VisitTargetEntryInstr(TargetEntryInstr* instr) -> bool override {
    ASSERT(instr);
    SetBlock(instr);
    Append(instr);
    InstructionIterator iter(instr->GetFirstInstruction());
    while (iter.HasNext()) {
      const auto next = iter.Next();
      ASSERT(next);
      if (!next->Accept(this))
        return false;
    }
    return true;
  }

  auto VisitJoinEntryInstr(JoinEntryInstr* instr) -> bool override {
    ASSERT(instr);
    SetBlock(instr);
    Append(instr);
    InstructionIterator iter(instr->GetFirstInstruction());
    while (iter.HasNext()) {
      const auto next = iter.Next();
      ASSERT(next);
      if (!next->Accept(this))
        return false;
    }
    return true;
  }

  auto VisitConstantInstr(ConstantInstr* instr) -> bool override {
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

  auto VisitConsInstr(ConsInstr* instr) -> bool override {
    ASSERT(instr);
    Append(instr);
    return true;
  }

  auto VisitReturnInstr(ReturnInstr* instr) -> bool override {
    ASSERT(instr);
    Append(instr);
    return true;
  }

  auto VisitBinaryOpInstr(BinaryOpInstr* instr) -> bool override {
    ASSERT(instr);
    const auto node = Append(instr);
    ASSERT(node);
    std::stringstream label;
    label << instr->GetName() << std::endl;
    label << "Op: " << instr->GetOp();
    SetNodeLabel(node, label);
    return true;
  }

  auto VisitCallProcInstr(CallProcInstr* instr) -> bool override {
    ASSERT(instr);
    const auto node = Append(instr);
    ASSERT(node);
    const auto symbol = instr->GetSymbol();
    ASSERT(symbol);
    std::stringstream label;
    label << instr->GetName() << std::endl;
    label << "Symbol := " << symbol->Get();
    SetNodeLabel(node, label);
    return true;
  }

  auto VisitLoadVariableInstr(LoadVariableInstr* instr) -> bool override {
    ASSERT(instr);
    const auto node = Append(instr);
    ASSERT(node);
    const auto symbol = instr->GetSymbol();
    ASSERT(symbol);
    std::stringstream label;
    label << instr->GetName() << std::endl;
    label << "Symbol := " << symbol->Get();
    SetNodeLabel(node, label);
    return true;
  }

  auto VisitStoreVariableInstr(StoreVariableInstr* instr) -> bool override {
    ASSERT(instr);
    const auto node = Append(instr);
    ASSERT(node);
    const auto symbol = instr->GetSymbol();
    ASSERT(symbol);
    std::stringstream label;
    label << instr->GetName() << std::endl;
    label << "Symbol := " << symbol->Get();
    SetNodeLabel(node, label);
    return true;
  }

  auto VisitGotoInstr(GotoInstr* instr) -> bool override {
    ASSERT(instr);
    Append(instr);
    return true;
  }

  auto VisitThrowInstr(ThrowInstr* instr) -> bool override {
    ASSERT(instr);
    Append(instr);
    return true;
  }

  auto VisitUnaryOpInstr(UnaryOpInstr* instr) -> bool override {
    ASSERT(instr);
    Append(instr);
    return true;
  }

  auto VisitBranchInstr(BranchInstr* instr) -> bool override {
    ASSERT(instr);
    Append(instr);

    DotVisitor for_true(GetOwner(), GetGraph());
    if (!instr->GetTrueTarget()->Accept(&for_true))
      return false;
    if (HasExitNode() && for_true.HasEntryNode()) {
      const auto edge_id = fmt::format("blk{0:d}blk{1:d}", GetBlock()->GetBlockId(), for_true.GetBlock()->GetBlockId());
      const auto edge = NewEdge(GetExitNode(), for_true.GetEntryNode(), edge_id.c_str());
      ASSERT(edge);
      SetEdgeLabel(edge, "is true");
    }

    DotVisitor for_false(GetOwner(), GetGraph());
    if (instr->HasFalseTarget()) {
      if (!instr->GetFalseTarget()->Accept(&for_false))
        return false;
      if (HasExitNode() && for_false.HasEntryNode()) {
        const auto edge_id = fmt::format("blk{0:d}blk{1:d}", GetBlock()->GetBlockId(), for_false.GetBlock()->GetBlockId());
        const auto edge = NewEdge(GetExitNode(), for_false.GetEntryNode(), edge_id.c_str());
        ASSERT(edge);
        SetEdgeLabel(edge, "is false");
      }
    }

    DotVisitor join(GetOwner(), GetGraph());
    if (!instr->GetJoin()->Accept(&join))
      return false;
    if (for_true.HasExitNode() && join.HasEntryNode()) {
      const auto edge_id = fmt::format("blk{0:d}blk{1:d}", for_true.GetBlock()->GetBlockId(), join.GetBlock()->GetBlockId());
      const auto edge = NewEdge(for_true.GetExitNode(), join.GetEntryNode(), edge_id.c_str());
      ASSERT(edge);
    }

    if (for_false.HasExitNode() && join.HasEntryNode()) {
      const auto edge_id = fmt::format("blk{0:d}blk{1:d}", for_false.GetBlock()->GetBlockId(), join.GetBlock()->GetBlockId());
      const auto edge = NewEdge(for_false.GetExitNode(), join.GetEntryNode(), edge_id.c_str());
      ASSERT(edge);
    }
    SetExitNode(join.GetExitNode());
    return true;
  }
};

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
  DotVisitor vis(this, GetGraph());
  if (!graph_entry->Accept(&vis)) {
    LOG(ERROR) << "failed to visit: " << graph_entry;
    return nullptr;
  }
  return dot::Graph::New(this);
}
}  // namespace scm