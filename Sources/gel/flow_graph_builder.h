#ifndef GEL_FLOW_GRAPH_BUILDER_H
#define GEL_FLOW_GRAPH_BUILDER_H

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "common.h"
#include "constructor.h"
#include "expr/clause_expr.h"
#include "expr/expression.h"
#include "flags.h"
#include "flow_graph.h"
#include "instruction.h"
#include "lambda.h"
#include "local.h"
#include "local_scope.h"
#include "object.h"
#include "platform.h"
#include "type.h"
#include "type_traits.h"

namespace gel {
template <class I>
concept IsEntryInstr = requires(const uword idx) {
  { I::New(idx) } -> std::convertible_to<EntryInstr*>;
};

template <class T>
concept HasBodyExpr = requires(T value) {
  { value.GetBody() } -> std::convertible_to<expr::Expression*>;
};

class FlowGraphBuilder {
  friend class ValueVisitor;
  friend class EffectVisitor;
  friend class CondClauseEffectVisitor;
  DEFINE_NON_COPYABLE_TYPE(FlowGraphBuilder);

 private:
  LocalScope* scope_ = nullptr;
  GraphEntryInstr* entry_ = nullptr;
  EntryInstr* block_ = nullptr;
  uint64_t num_blocks_ = 0;
  uint64_t num_temps_ = 0;

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

  inline auto PushScope() -> LocalScope* {
    const auto new_scope = LocalScope::New(GetScope());
    SetScope(new_scope);
    return new_scope;
  }

  inline auto PushScope(const std::vector<LocalScope*>& scopes) -> LocalScope* {
    const auto new_scope = LocalScope::Union(scopes, GetScope());
    SetScope(new_scope);
    return new_scope;
  }

  inline void PopScope() {
    ASSERT(HasScope());
    SetScope(GetScope()->GetParent());
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

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
  }

  auto GetGraphEntry() const -> GraphEntryInstr* {
    return entry_;
  }

  auto HasGraphEntry() const -> bool {
    return GetGraphEntry() != nullptr;
  }

 public:
  template <HasBodyExpr Target>
  static auto Build(Target& target, LocalScope* scope = LocalScope::New()) -> FlowGraph*;
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
  virtual void Do(ir::Definition* defn) {
    ASSERT(defn);
    if (IsEmpty()) {
      SetEntryInstr(defn);
    } else {
      Instruction::Link(GetExitInstr(), defn);
    }
    SetExitInstr(defn);
  }

  virtual void ReturnDefinition(ir::Definition* defn) {
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

  void AddInstanceOf(ir::Definition* defn, Class* expected);
  auto CreateCallFor(ir::Definition* defn, const uword num_args) -> ir::Definition*;
  auto CreateStoreLoad(LocalVariable* local, ir::Definition* value) -> ir::Definition*;
  auto CreateCastTo(ir::Definition* value, Class* target) -> ir::Definition*;

  inline auto DoCastTo(ir::Definition* defn, Class* expected) -> ir::Definition* {
    ASSERT(defn);
    ASSERT(expected);
    const auto casted = ir::CastInstr::New(defn, expected);
    Do(casted);
    return casted;
  }

  inline void AddReturnExit(ir::Definition* value) {
    Add(ir::ReturnInstr::New(value));
    exit_ = nullptr;
  }

  inline void AddReturnExit(Object* rhs) {
    ASSERT(rhs);
    return AddReturnExit(Bind(ir::ConstantInstr::New(rhs)));
  }

  inline void AddThrow(ir::Definition* defn) {
    ASSERT(defn);
    if (gel::IsPedantic())
      AddInstanceOf(defn, Error::GetClass());
    return Add(ir::ThrowInstr::New(defn));
  }

  inline void AddThrow(Error* rhs) {
    ASSERT(rhs);
    return AddThrow(Bind(ir::ConstantInstr::New(rhs)));
  }

  inline void AddThrow(const std::string& message) {
    ASSERT(!message.empty());
    return AddThrow(Error::New(message));
  }

  auto ReturnCall(ir::InvokeInstr* defn) -> bool;
  auto ReturnCallTo(ir::Definition* defn, const uword num_args) -> bool;
  auto ReturnCallTo(Fn* procedure, const uword num_args) -> bool;

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

  auto Bind(ir::Definition* defn) -> ir::Definition* {
    if (IsEmpty()) {
      SetEntryInstr(defn);
    } else {
      Instruction::Link(GetExitInstr(), defn);
    }
    SetExitInstr(defn);
    return defn;
  }

  template <IsEntryInstr I>
  inline auto NewBlock() -> I* {
    const auto parent = GetCurrentBlock();
    const auto blk = I::New(GetOwner()->GetNextBlockId());
    if (parent)
      parent->AddDominated(blk);
    SetCurrentBlock(blk);
    return blk;
  }

  inline void SetCurrentBlock(EntryInstr* instr) {
    ASSERT(instr);
    block_ = instr;
  }

  inline auto GetCurrentBlock() const -> EntryInstr* {
    return block_;
  }

  inline void AddImplicitReturn() {
    ASSERT(IsOpen());
    if (GetExitInstr()->IsDefinition())
      return AddReturnExit(GetExitInstr()->AsDefinition());
    return AddReturnExit(Bind(gel::ConstantInstr::New(gel::Nil::Get())));
  }

  virtual void ReturnValue(ir::Definition* defn) {}
  void GenerateDefaultImplementation(Lambda* lambda);

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

  auto IsClosed() const -> bool {
    return !IsOpen();
  }

  template <HasBodyExpr T>
  auto Visit(T& rhs) -> bool;
#define DECLARE_VISIT(Name) virtual auto Visit##Name(Name* name) -> bool override;
  FOR_EACH_EXPRESSION_NODE(DECLARE_VISIT)
#undef DECLARE_VISIT

  auto operator()(expr::Expression* expr) -> bool {
    ASSERT(expr);
    return expr->Accept(*this);
  }

  template <HasBodyExpr T>
  inline auto operator()(T& rhs) -> bool {
    return Visit(rhs);
  }
};

class ValueVisitor : public EffectVisitor {
  DEFINE_NON_COPYABLE_TYPE(ValueVisitor);

 private:
  ir::Definition* value_ = nullptr;

 protected:
  void ReturnValue(ir::Definition* value) override {
    ASSERT(value);
    value_ = value;
  }

  void ReturnDefinition(ir::Definition* defn) override {
    value_ = Bind(defn);
  }

  inline void ReturnNull() {
    return ReturnDefinition(ir::ConstantInstr::New(Nil::Get()));
  }

 public:
  explicit ValueVisitor(FlowGraphBuilder* owner) :
    EffectVisitor(owner) {}
  ~ValueVisitor() override = default;

  auto GetValue() const -> ir::Definition* {
    return value_;
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  auto VisitDoExpr(expr::DoExpr* expr) -> bool override;
  auto VisitSeqExpr(expr::SeqExpr* expr) -> bool override;
  auto VisitBindingExpr(expr::BindingExpr* expr) -> bool override;

  auto operator()(expr::Expression* rhs) -> bool {
    ASSERT(rhs);
    return rhs->Accept(*this);
  }
};

class CondClauseEffectVisitor : public EffectVisitor {
  DEFINE_NON_COPYABLE_TYPE(CondClauseEffectVisitor);

 private:
  ir::TargetEntryInstr* target_;
  ir::JoinEntryInstr* join_;

 public:
  explicit CondClauseEffectVisitor(FlowGraphBuilder* owner, ir::TargetEntryInstr* target, ir::JoinEntryInstr* join) :
    EffectVisitor(owner),
    target_(target),
    join_(join) {
    ASSERT(target_);
    ASSERT(join_);
  }
  ~CondClauseEffectVisitor() override = default;

  auto GetTarget() const -> ir::TargetEntryInstr* {
    return target_;
  }

  auto GetJoin() const -> ir::JoinEntryInstr* {
    return join_;
  }

  auto VisitClauseExpr(expr::ClauseExpr* expr) -> bool override;

  auto operator()(expr::ClauseExpr* expr) -> bool {
    ASSERT(expr);
    return expr->Accept(*this);
  }
};
}  // namespace gel

#endif  // GEL_FLOW_GRAPH_BUILDER_H
