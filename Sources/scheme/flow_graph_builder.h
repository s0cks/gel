#ifndef SCM_FLOW_GRAPH_BUILDER_H
#define SCM_FLOW_GRAPH_BUILDER_H

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/flow_graph.h"
#include "scheme/instruction.h"
#include "scheme/object.h"

namespace scm {
class FlowGraphBuilder {
  friend class ClauseVisitor;
  friend class EffectVisitor;
  DEFINE_NON_COPYABLE_TYPE(FlowGraphBuilder);

 private:
  LocalScope* scope_ = nullptr;
  GraphEntryInstr* entry_ = nullptr;
  EntryInstr* block_ = nullptr;
  uint64_t num_blocks_ = 0;

  inline void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

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
  explicit FlowGraphBuilder(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
  }
  ~FlowGraphBuilder() = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetGraphEntry() const -> GraphEntryInstr* {
    return entry_;
  }

  auto HasGraphEntry() const -> bool {
    return GetGraphEntry() != nullptr;
  }

 public:
  static auto Build(Expression* expr, LocalScope* scope) -> FlowGraph*;
  static auto Build(Script* script, LocalScope* scope) -> FlowGraph*;
};

class ValueVisitor;
class EffectVisitor : public ExpressionVisitor {
  friend class FlowGraphBuilder;
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

  void AddInstanceOf(instr::Definition* defn, Class* expected);
  auto CreateCallFor(instr::Definition* defn, const uword num_args) -> instr::InvokeInstr*;
  auto CreateStoreLoad(Symbol* symbol, instr::Definition* value) -> instr::Definition*;
  auto CreateCastTo(instr::Definition* value, Class* target) -> instr::Definition*;

  inline auto DoCastTo(instr::Definition* defn, Class* expected) -> instr::Definition* {
    ASSERT(defn);
    ASSERT(expected);
    const auto casted = instr::CastInstr::New(defn, expected);
    Do(casted);
    return casted;
  }

  inline void AddReturnExit(instr::Definition* value) {
    Add(ReturnInstr::New(value));
    exit_ = nullptr;
  }

  auto ReturnCall(instr::InvokeInstr* defn) -> bool;
  auto ReturnCall(Procedure* procedure, const uword num_args) -> bool;
  auto ReturnCall(Symbol* function, const uword num_args) -> bool;

  void Append(const EffectVisitor& rhs) {
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

  inline auto CreateReturnForExit(instr::Instruction* exit_instr) -> instr::ReturnInstr* {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    return exit_instr->IsDefinition() ? ReturnInstr::New((instr::Definition*)exit_instr) : ReturnInstr::New();
  }

  inline void AddImplicitReturn() {
    const auto exit = GetExitInstr();
    if (exit && !exit->IsReturnInstr())
      Add(CreateReturnForExit(exit));
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

  auto VisitScript(Script* script) -> bool;
#define DECLARE_VISIT(Name) virtual auto Visit##Name(Name* name)->bool override;
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

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }
};

class RxEffectVisitor : public EffectVisitor {
  DEFINE_NON_COPYABLE_TYPE(RxEffectVisitor);

 private:
  instr::Definition* observable_;

 public:
  RxEffectVisitor(FlowGraphBuilder* owner, instr::Definition* observable) :
    EffectVisitor(owner),
    observable_(observable) {
    ASSERT(observable_);
  }
  ~RxEffectVisitor() override = default;

  auto GetObservable() const -> instr::Definition* {
    return observable_;
  }

  auto VisitRxOpExpr(expr::RxOpExpr* expr) -> bool override;
};
}  // namespace scm

#endif  // SCM_FLOW_GRAPH_BUILDER_H
