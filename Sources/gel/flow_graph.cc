#include "gel/flow_graph.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include "gel/bitvector.h"
#include "gel/common.h"
#include "gel/instruction.h"
#include "gel/to_string_helper.h"

namespace gel {
auto FlowGraph::Accept(InstructionVisitor* vis) const -> bool {
  ASSERT(vis);
  InstructionIterator iter(GetEntry());
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    if (!next->Accept(vis))
      return false;
  }
  return true;
}

struct BlockTraversalState {
  ir::EntryInstr* block;
  word successor;

  explicit BlockTraversalState(ir::EntryInstr* blk) :
    block(blk),
    successor(static_cast<word>(blk->GetLastInstruction()->GetSuccessorCount()) - 1) {}
  ~BlockTraversalState() = default;

  auto HasSuccessor() const -> bool {
    return successor >= 0;
  }

  auto NextSuccessor() -> ir::EntryInstr* {
    ASSERT(HasSuccessor());
    return block->GetLastInstruction()->GetSuccessorAt(successor--);
  }

  friend auto operator<<(std::ostream& stream, const BlockTraversalState& rhs) -> std::ostream& {
    ToStringHelper<BlockTraversalState> helper{};
    helper.AddField("block", rhs.block);
    helper.AddField("success", rhs.successor);
    return stream << helper.ToString();
  }

  DEFINE_DEFAULT_COPYABLE_TYPE(BlockTraversalState);
};

void FlowGraph::DiscoverBlocks() {
  ASSERT(HasEntry());
  preorder_.clear();
  postorder_.clear();
  reverse_postorder_.clear();
  parent_.clear();

  std::vector<BlockTraversalState> blocks{};
  GetEntry()->DiscoverBlocks(nullptr, preorder_, parent_);

  BlockTraversalState init_state(GetEntry());
  blocks.push_back(init_state);
  while (!blocks.empty()) {
    auto& state = blocks.back();
    DLOG(INFO) << "processing: " << state;
    if (state.HasSuccessor()) {
      const auto successor = state.NextSuccessor();
      DLOG(INFO) << "successor: " << successor->ToString();
      if (successor->DiscoverBlocks(state.block, preorder_, parent_)) {
        blocks.emplace_back(successor);
      }
    } else {
      blocks.pop_back();
      state.block->SetPostorderNum(static_cast<word>(postorder_.size()));
      postorder_.push_back(state.block);
    }
  }
  ASSERT(postorder_.size() == preorder_.size());

  const auto count = postorder_.size();
  for (auto idx = 0; idx < count; idx++) {
    const auto blk = postorder_[count - idx - 1];
    reverse_postorder_.push_back(blk);
  }
}

void FlowGraph::ComputeSSA(const uword num_vregs) {
  std::vector<BitVector*> dominance{};
  ComputeDominators(dominance);
}

template <typename T>
static inline auto Minimum(const T a, const T b) -> T {
  return a < b ? a : b;
}

static inline void Compress(const word start, const word current, std::vector<word>& parent, std::vector<word>& label) {
  const auto next = parent[current];
  if (next > start) {
    Compress(start, next, parent, label);
    label[current] = Minimum(label[current], label[next]);
    parent[current] = parent[next];
  }
}

void FlowGraph::ComputeDominators(std::vector<BitVector*>& dominators) {
  const auto size = static_cast<word>(parent_.size());
  std::vector<word> idom(size);
  std::vector<word> semi(size);
  std::vector<word> label(size);
  dominators.reserve(size);

  for (auto idx = 0; idx < size; idx++) {
    idom.push_back(parent_[idx]);
    semi.push_back(idx);
    label.push_back(idx);
    dominators.push_back(new BitVector(static_cast<word>(size)));
  }

  preorder_[0]->ClearDominated();
  for (word idx = size - 1; idx >= 1; idx--) {
    auto blk = preorder_[idx];
    blk->ClearDominated();
    for (auto i = 0; i < blk->GetNumberOfPredecessors(); i++) {
      auto predecessor = blk->GetPredecessorAt(i);
      const auto pred_idx = predecessor->GetPreorderNum();
      auto best = pred_idx;
      if (pred_idx > idx) {
        Compress(idx, pred_idx, parent_, label);
        best = label[pred_idx];
      }
      semi[idx] = Minimum(semi[idx], semi[best]);
    }
    label[idx] = semi[idx];
  }

  for (word idx = 1; idx < size; idx++) {
    auto dom_index = idom[idx];
    while (dom_index > semi[idx]) {
      dom_index = idom[dom_index];
    }
    idom[idx] = dom_index;
    preorder_[dom_index]->AddDominated(preorder_[idx]);
  }

  for (word idx = 0; idx < size; idx++) {
    const auto blk = preorder_[idx];
    const auto count = blk->GetNumberOfPredecessors();
    if (count <= 1)
      continue;
    for (word i = 0; i < count; i++) {
      auto runner = blk->GetPredecessorAt(i);
      while (runner != blk->GetDominator()) {
        dominators[runner->GetPreorderNum()]->Add(i);
        runner = runner->GetDominator();
      }
    }
  }
}

void FlowGraph::InsertPhis(std::vector<BitVector*>& assigned, std::vector<BitVector*>& df, std::vector<PhiInstr*>& live_phis) {}
}  // namespace gel
