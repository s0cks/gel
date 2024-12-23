#ifndef GEL_FLOW_GRAPH_DOT_H
#define GEL_FLOW_GRAPH_DOT_H

#ifdef GEL_ENABLE_GV

#include <fmt/format.h>

#include <vector>

#include "gel/common.h"
#include "gel/flow_graph.h"
#include "gel/gv.h"
#include "gel/instruction.h"

namespace gel {
namespace dot {
class BlockVisitor;
class EffectVisitor;
}  // namespace dot

class FlowGraphToDotGraph : public dot::GraphBuilder {
  friend class dot::BlockVisitor;
  friend class dot::EffectVisitor;
  DEFINE_NON_COPYABLE_TYPE(FlowGraphToDotGraph);

 private:
  uint64_t num_instructions_ = 0;
  const FlowGraph* flow_graph_;
  NodeList nodes_{};
  EdgeList edges_{};
  Node* previous_ = nullptr;
  ir::EntryInstr* block_ = nullptr;

  inline void SetBlock(ir::EntryInstr* blk) {
    ASSERT(blk);
    block_ = blk;
  }

  inline void SetPrevious(Node* previous) {
    ASSERT(previous);
    previous_ = previous;
  }

  inline auto GetPrevious() const -> Node* {
    return previous_;
  }

  inline auto HasPrevious() const -> bool {
    return GetPrevious() != nullptr;
  }

  inline auto NewNode(Instruction* instr) -> Node* {
    ASSERT(instr);
    const auto node_id = fmt::format("i{0:d}", num_instructions_++);
    const auto node = GraphBuilder::NewNode(node_id);
    ASSERT(node);
    SetNodeLabel(node, instr->GetName());
    return node;
  }

  inline auto CreateNode(const std::string& node_id) -> Node* {
    const auto node = GraphBuilder::NewNode(node_id);
    ASSERT(node);
    nodes_.push_back(node);
    return node;
  }

  inline auto CreateNode() -> Node* {
    const auto node_id = fmt::format("b{0:d}n{1:d}", GetBlock()->GetBlockId(), nodes_.size() + 1);
    return CreateNode(node_id);
  }

  inline auto CreateNode(EntryInstr* instr) -> Node* {
    ASSERT(instr);
    return CreateNode(fmt::format("blk{}", instr->GetBlockId()));
  }

 public:
  explicit FlowGraphToDotGraph(const char* name, const FlowGraph* flow_graph) :
    dot::GraphBuilder(name, Agdirected),
    flow_graph_(flow_graph) {
    ASSERT(flow_graph);
  }
  ~FlowGraphToDotGraph() override = default;

  auto GetFlowGraph() const -> const FlowGraph* {
    return flow_graph_;
  }

  auto HasFlowGraph() const -> bool {
    return GetFlowGraph() != nullptr;
  }

  auto GetGraphEntryInstr() const -> GraphEntryInstr* {
    return GetFlowGraph()->GetEntry();
  }

  auto GetBlock() const -> ir::EntryInstr* {
    return block_;
  }

  auto Build() -> dot::Graph* override;

 public:
  static inline auto BuildGraph(const char* name, const FlowGraph* flow_graph) -> dot::Graph* {
    ASSERT(name);
    ASSERT(flow_graph);
    FlowGraphToDotGraph builder(name, flow_graph);
    const auto dot_graph = builder.Build();
    ASSERT(dot_graph);
    return dot_graph;
  }
};

namespace dot {
class EffectVisitor : public InstructionVisitor, public dot::GraphDecorator {
  DEFINE_NON_COPYABLE_TYPE(EffectVisitor);

 private:
  FlowGraphToDotGraph* owner_;
  Node* entry_ = nullptr;
  Node* exit_ = nullptr;

 protected:
  inline void SetEntry(Node* node) {
    ASSERT(node);
    entry_ = node;
  }

  inline void SetExit(Node* node) {
    ASSERT(node);
    exit_ = node;
  }

  inline void Append(Node* node, const bool create_edge = true) {
    ASSERT(node);
    if (!HasEntry()) {
      SetEntry(node);
      SetExit(node);
      return;
    }

    if (HasExit() && create_edge) {
      const auto previous = GetExit();
      const auto edge = NewEdge(previous, node, "");
      ASSERT(edge);
    }
    SetExit(node);
  }

  inline auto Append(Instruction* instr, const bool create_edge = true) -> Node* {
    ASSERT(instr);
    const auto node = instr->IsEntryInstr() ? GetOwner()->CreateNode((EntryInstr*)instr) : GetOwner()->CreateNode();
    ASSERT(node);
    Append(node, create_edge);
    std::stringstream label;
    label << instr->GetName();
    SetNodeLabel(node, label);
    return node;
  }

  inline auto GetCurrentBlock() const -> ir::EntryInstr* {
    return GetOwner()->GetBlock();
  }

  static inline auto GetBlockNodeId(ir::EntryInstr* instr) -> std::string {
    ASSERT(instr);
    return fmt::format("blk{}", instr->GetBlockId());
  }

  inline auto SeenBlock(ir::EntryInstr* instr) -> bool {
    return HasNode(GetBlockNodeId(instr));
  }

  inline auto GetBlockNode(ir::EntryInstr* instr) -> Node* {
    return GetNode(GetBlockNodeId(instr));
  }

 public:
  explicit EffectVisitor(FlowGraphToDotGraph* owner) :
    InstructionVisitor(),
    dot::GraphDecorator(owner->GetGraph()),
    owner_(owner) {
    ASSERT(owner_);
  }
  ~EffectVisitor() override = default;

  auto GetOwner() const -> FlowGraphToDotGraph* {
    return owner_;
  }

  auto GetEntry() const -> Node* {
    return entry_;
  }

  inline auto HasEntry() const -> bool {
    return GetEntry() != nullptr;
  }

  auto GetExit() const -> Node* {
    return exit_;
  }

  inline auto HasExit() const -> bool {
    return GetExit() != nullptr;
  }

#define DECLARE_VISIT(Name) auto Visit##Name(Name##Instr* instr)->bool override;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT
};

class BlockVisitor : public EffectVisitor {
  DEFINE_NON_COPYABLE_TYPE(BlockVisitor);

 public:
  explicit BlockVisitor(FlowGraphToDotGraph* owner) :
    EffectVisitor(owner) {}
  ~BlockVisitor() override = default;

  auto VisitGraphEntryInstr(GraphEntryInstr* instr) -> bool override;
  auto VisitTargetEntryInstr(TargetEntryInstr* instr) -> bool override;
  auto VisitJoinEntryInstr(JoinEntryInstr* instr) -> bool override;
};
}  // namespace dot
}  // namespace gel

#endif  // GEL_ENABLE_GV
#endif  // GEL_FLOW_GRAPH_DOT_H
