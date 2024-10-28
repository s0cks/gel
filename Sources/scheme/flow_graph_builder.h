#ifndef SCM_FLOW_GRAPH_BUILDER_H
#define SCM_FLOW_GRAPH_BUILDER_H

#include "scheme/expression.h"
#include "scheme/flow_graph.h"
#include "scheme/instruction.h"
#include "scheme/program.h"

namespace scm {
class FlowGraphBuilder {
  friend class EffectVisitor;
  DEFINE_NON_COPYABLE_TYPE(FlowGraphBuilder);

 private:
  Expression* expr_;
  GraphEntryInstr* entry_ = nullptr;
  EntryInstr* block_ = nullptr;
  uint64_t num_blocks_ = 0;

  inline void SetCurrentBlock(EntryInstr* instr) {
    ASSERT(instr);
    block_ = instr;
  }

  inline auto GetCurrentBlock() const -> EntryInstr* {
    return block_;
  }

  inline void SetGraphEntry(GraphEntryInstr* instr) {
    ASSERT(instr);
    entry_ = instr;
  }

  inline auto GetNextBlockId() -> uint64_t {
    const auto next = num_blocks_;
    num_blocks_++;
    return next;
  }

 public:
  explicit FlowGraphBuilder(Expression* expr) :
    expr_(expr) {}
  ~FlowGraphBuilder() = default;

  auto GetExpr() const -> Expression* {
    return expr_;
  }

  auto GetGraphEntry() const -> GraphEntryInstr* {
    return entry_;
  }

  auto HasGraphEntry() const -> bool {
    return GetGraphEntry() != nullptr;
  }

  auto BuildGraph() -> FlowGraph*;

 public:
  static inline auto Build(Expression* expr) -> FlowGraph* {
    ASSERT(expr);
    FlowGraphBuilder builder(expr);
    return builder.BuildGraph();
  }

  static inline auto Build(Program* program) -> FlowGraph* {
    ASSERT(program);
    ASSERT(program->GetNumberOfExpressions() >= 1);
    FlowGraphBuilder builder(program->GetExpressionAt(0));
    return builder.BuildGraph();
  }
};

class EffectVisitor : public ExpressionVisitor {
  DEFINE_NON_COPYABLE_TYPE(EffectVisitor);

 private:
  FlowGraphBuilder* owner_;
  Instruction* entry_ = nullptr;
  Instruction* exit_ = nullptr;
  EntryInstr* block_ = nullptr;

 protected:
  virtual void Do(instr::Definition* defn) {
    ASSERT(defn);
    if (IsEmpty()) {
      SetEntryInstr(defn);
    } else {
      Instruction::Link(GetExitInstr(), defn);
    }
    SetExitInstr(defn);
  }

  virtual void ReturnDefinition(instr::Definition* defn) {
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

  inline void AddReturnExit(instr::Definition* value) {
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

  auto Bind(instr::Definition* defn) -> instr::Definition* {
    if (IsEmpty()) {
      SetEntryInstr(defn);
    } else {
      Instruction::Link(GetExitInstr(), defn);
    }
    SetExitInstr(defn);
    return defn;
  }

  inline void SetCurrentBlock(EntryInstr* instr) {
    ASSERT(instr);
    block_ = instr;
  }

  inline auto GetCurrentBlock() const -> EntryInstr* {
    return block_;
  }

  virtual void ReturnValue(instr::Definition* defn) {}

 public:
  explicit EffectVisitor(FlowGraphBuilder* owner) :
    ExpressionVisitor(),
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

#define DECLARE_VISIT(Name) virtual auto Visit##Name(Name##Expr* name) -> bool override;
  FOR_EACH_EXPRESSION_NODE(DECLARE_VISIT)
#undef DECLARE_VISIT
};

class ValueVisitor : public EffectVisitor {
  DEFINE_NON_COPYABLE_TYPE(ValueVisitor);

 private:
  instr::Definition* value_ = nullptr;

 protected:
  void ReturnValue(instr::Definition* value) override {
    ASSERT(value);
    value_ = value;
  }

  void ReturnDefinition(instr::Definition* defn) override {
    value_ = Bind(defn);
  }

 public:
  explicit ValueVisitor(FlowGraphBuilder* owner) :
    EffectVisitor(owner) {}
  ~ValueVisitor() override = default;

  auto GetValue() const -> instr::Definition* {
    return value_;
  }
};
}  // namespace scm

#endif  // SCM_FLOW_GRAPH_BUILDER_H
