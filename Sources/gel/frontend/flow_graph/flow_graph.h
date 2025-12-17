#ifndef GEL_FLOW_GRAPH_H
#define GEL_FLOW_GRAPH_H

#include <vector>

#include "gel/bitvector.h"
#include "gel/common.h"
#include "gel/instruction.h"
#include "gel/type/object.h"

namespace gel {
class FlowGraph {
  friend class FlowGraphBuilder;
  DEFINE_NON_COPYABLE_TYPE(FlowGraph);

 private:
  Object* target_;
  GraphEntryInstr* entry_;
  uword current_temp_index_ = 0;
  std::vector<word> parent_{};
  std::vector<ir::EntryInstr*> preorder_{};
  std::vector<ir::EntryInstr*> postorder_{};
  std::vector<ir::EntryInstr*> reverse_postorder_{};

  void ComputeDominators(std::vector<BitVector*>& dominators);
  void InsertPhis(std::vector<BitVector*>& assigned, std::vector<BitVector*>& df, std::vector<PhiInstr*>& live_phis);

 public:
  FlowGraph(Object* target, GraphEntryInstr* entry) :
    target_(target),
    entry_(entry) {
    ASSERT(target_);
    ASSERT(entry);
  }
  ~FlowGraph() = default;

  auto GetTarget() const -> Object* {
    return target_;
  }

  auto GetEntry() const -> GraphEntryInstr* {
    return entry_;
  }

  auto HasEntry() const -> bool {
    return GetEntry() != nullptr;
  }

  auto GetPreorder() const -> const std::vector<ir::EntryInstr*>& {
    return preorder_;
  }

  auto GetPostorder() const -> const std::vector<ir::EntryInstr*>& {
    return postorder_;
  }

  auto GetReversePostorder() const -> const std::vector<ir::EntryInstr*>& {
    return reverse_postorder_;
  }

  auto Accept(InstructionVisitor* vis) const -> bool;
  void DiscoverBlocks();
  void ComputeSSA(const uword num_vregs);
};
}  // namespace gel

#endif  // GEL_FLOW_GRAPH_H
