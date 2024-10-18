#include "scheme/flow_graph_dot.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include <sstream>

#include "scheme/flow_graph.h"
#include "scheme/instruction.h"

namespace scm {
void FlowGraphToDotGraph::Set(Node* node, const std::string& name, const std::string& value) {
  ASSERT(node);
  ASSERT(!name.empty());
  ASSERT(!value.empty());
  const auto val = agstrdup(GetGraph(), value.c_str());
  agset(node, const_cast<char*>(name.c_str()), val);
  agstrfree(GetGraph(), val);
  DLOG(INFO) << "set attr " << name << " to: " << value;
}

auto FlowGraphToDotGraph::CreateXLabel(Instruction* instr) -> std::optional<std::string> {
  ASSERT(instr);
  std::stringstream ss;
  if (instr->IsConstantInstr()) {
    const auto value = instr->AsConstantInstr()->GetValue();
    ASSERT(value);
    ss << "value=" << value->ToString();
    return {ss.str()};
  } else if (instr->IsStoreVariableInstr()) {
    const auto var = instr->AsStoreVariableInstr()->GetVariable();
    ASSERT(var);
    ss << "var=" << var->GetName();
    return {ss.str()};
  }
  return std::nullopt;
}

auto FlowGraphToDotGraph::CreateNextNode(Instruction* instr) -> Node* {
  ASSERT(instr);
  const auto node_id = fmt::format("i{0:d}", ++num_instructions_);
  DLOG(INFO) << node_id << " := " << instr->ToString();
  const auto node = NewNode(node_id);
  ASSERT(node);
  SetLabel(node, instr->GetName());
  const auto xlabel = CreateXLabel(instr);
  if (xlabel)
    SetXLabel(node, *xlabel);
  return node;
}

void FlowGraphToDotGraph::InitEdges() {
  // create edges
  std::deque<Node*> work(std::begin(nodes_), std::end(nodes_));
  const auto pop = [&work]() {
    const auto current = work.front();
    ASSERT(current);
    work.pop_front();
    return current;
  };

  do {
    const auto current = pop();
    if (work.empty())
      return;
    const auto next = pop();
    const auto edge = NewEdge(current, next);
    ASSERT(edge);
    work.push_front(next);
    edges_.push_back(edge);
  } while (!work.empty());
}

void FlowGraphToDotGraph::InitNodes() {
  InstructionIterator iter(GetGraphEntryInstr());
  while (iter.HasNext()) {
    const auto next = CreateNextNode(iter.Next());
    ASSERT(next);
    nodes_.push_back(next);
  }
}

auto FlowGraphToDotGraph::BuildDotGraph() -> DotGraph* {
  ASSERT(HasFlowGraph());
  agattr(GetGraph(), AGNODE, "label", "");
  agattr(GetGraph(), AGNODE, "xlabel", "");
  InitNodes();
  InitEdges();
  return DotGraph::New(GetGraph());
}
}  // namespace scm