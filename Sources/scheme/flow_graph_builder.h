#ifndef SCM_FLOW_GRAPH_BUILDER_H
#define SCM_FLOW_GRAPH_BUILDER_H

#include "scheme/flow_graph.h"

namespace scm {
class FlowGraphBuilder {
  DEFINE_NON_COPYABLE_TYPE(FlowGraphBuilder);

 private:
  GraphEntryInstr* entry_ = nullptr;
  ast::Program* program_;

  inline void SetGraphEntry(GraphEntryInstr* instr) {
    ASSERT(instr);
    entry_ = instr;
  }

 public:
  explicit FlowGraphBuilder(ast::Program* p) :
    program_(p) {
    ASSERT(p);
  }
  ~FlowGraphBuilder() = default;

  auto GetGraphEntry() const -> GraphEntryInstr* {
    return entry_;
  }

  auto HasGraphEntry() const -> bool {
    return GetGraphEntry() != nullptr;
  }

  auto GetProgram() const -> ast::Program* {
    return program_;
  }

  auto BuildGraph() -> FlowGraph*;
};

class EffectVisitor : public ast::NodeVisitor {
  DEFINE_NON_COPYABLE_TYPE(EffectVisitor);

 private:
  FlowGraphBuilder* owner_;
  Instruction* entry_ = nullptr;
  Instruction* exit_ = nullptr;

 protected:
  virtual void Do(Definition* defn) {
    ASSERT(defn);
    if (IsEmpty()) {
      SetEntryInstr(defn);
    } else {
      Instruction::Link(GetExitInstr(), defn);
    }
    SetExitInstr(defn);
  }

  virtual void ReturnDefinition(Definition* defn) {
    ASSERT(defn);
    if (!defn->IsConstantInstr())
      Do(defn);
  }

  inline void SetEntryInstr(Instruction* instr) {
    entry_ = instr;
  }

  inline void SetExitInstr(Instruction* instr) {
    exit_ = instr;
  }

  inline void Add(Instruction* instr) {
    ASSERT(instr);
    if (IsEmpty()) {
      SetEntryInstr(instr);
      SetExitInstr(instr);
    } else {
      Instruction::Link(GetExitInstr(), instr);
      SetExitInstr(instr);
    }
  }

  inline void AddReturnExit(Definition* value) {
    ASSERT(value);
    Add(new ReturnInstr(value));
    exit_ = nullptr;
  }

  void Append(EffectVisitor& rhs) {
    if (rhs.IsEmpty())
      return;
    if (IsEmpty()) {
      SetEntryInstr(rhs.GetEntryInstr());
    } else {
      Instruction::Link(GetExitInstr(), rhs.GetEntryInstr());
    }
    SetExitInstr(rhs.GetExitInstr());
  }

  auto Bind(Definition* defn) -> Definition* {
    if (IsEmpty()) {
      SetEntryInstr(defn);
    } else {
      Instruction::Link(GetExitInstr(), defn);
    }
    SetExitInstr(defn);
    return defn;
  }

  virtual void ReturnValue(Definition* defn) {}

 public:
  explicit EffectVisitor(FlowGraphBuilder* owner) :
    ast::NodeVisitor(),
    owner_(owner) {}
  ~EffectVisitor() override = default;

  auto GetOwner() const -> FlowGraphBuilder* {
    return owner_;
  }

  auto GetEntryInstr() const -> Instruction* {
    return entry_;
  }

  auto GetExitInstr() const -> Instruction* {
    return exit_;
  }

  auto IsEmpty() const -> bool {
    return GetEntryInstr() == nullptr;
  }

  auto IsOpen() const -> bool {
    return IsEmpty() || GetExitInstr() != nullptr;
  }

#define DECLARE_VISIT(Name) virtual auto Visit##Name(ast::Name* name) -> bool override;
  FOR_EACH_AST_NODE(DECLARE_VISIT)
#undef DECLARE_VISIT
};

class ValueVisitor : public EffectVisitor {
  DEFINE_NON_COPYABLE_TYPE(ValueVisitor);

 private:
  Definition* value_ = nullptr;

 protected:
  void ReturnValue(Definition* value) override {
    ASSERT(value);
    value_ = value;
  }

  void ReturnDefinition(Definition* defn) override {
    value_ = Bind(defn);
  }

 public:
  explicit ValueVisitor(FlowGraphBuilder* owner) :
    EffectVisitor(owner) {}
  ~ValueVisitor() override = default;

  auto GetValue() const -> Definition* {
    return value_;
  }
};
}  // namespace scm

#endif  // SCM_FLOW_GRAPH_BUILDER_H
