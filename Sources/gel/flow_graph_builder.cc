#include "gel/flow_graph_builder.h"

#include <glog/logging.h>

#include <algorithm>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/flags.h"
#include "gel/instruction.h"
#include "gel/lambda.h"
#include "gel/local.h"
#include "gel/module.h"
#include "gel/native_procedure.h"
#include "gel/natives.h"
#include "gel/object.h"
#include "gel/procedure.h"
#include "gel/rx.h"
#include "gel/script.h"

namespace gel {

template <class S>
class SeqExprIterator {
  DEFINE_NON_COPYABLE_TYPE(SeqExprIterator<S>);

 private:
  const EffectVisitor* owner_;
  uword index_ = 0;
  S* seq_;

 public:
  explicit SeqExprIterator(const EffectVisitor* owner, S* seq) :
    owner_(owner),
    seq_(seq) {
    ASSERT(owner_);
    ASSERT(seq_);
  }
  ~SeqExprIterator() = default;

  auto GetSeq() const -> S* {
    return seq_;
  }

  auto GetOwner() const -> const EffectVisitor* {
    return owner_;
  }

  auto HasNext() const -> bool {
    return GetOwner()->IsOpen() && GetCurrentIndex() < GetSeq()->GetNumberOfChildren();
  }

  auto GetCurrentIndex() const -> uword {
    return index_;
  }

  auto Next() -> std::pair<uword, expr::Expression*> {
    const auto next = std::make_pair(index_, GetSeq()->GetChildAt(index_));
    index_ += 1;
    return next;
  }
};

static inline auto AppendFragment(EntryInstr* entry, EffectVisitor& vis) -> Instruction* {
  ASSERT(entry);
  if (vis.IsEmpty())
    return entry;
  entry->Append(vis.GetEntryInstr());
  return vis.GetExitInstr();
}

static inline auto IsNativeCall(ir::Instruction* instr) -> bool {
  ASSERT(instr);
  if (!instr->IsConstantInstr())
    return false;
  const auto target = instr->AsConstantInstr()->GetValue();
  ASSERT(target);
  return target->IsNativeProcedure();
}

void EffectVisitor::AddInstanceOf(ir::Definition* defn, Class* cls) {
  ASSERT(defn);
  return Add(ir::InstanceOfInstr::New(defn, cls));
}

auto EffectVisitor::CreateCallFor(ir::Definition* defn, const uword num_args) -> ir::InvokeInstr* {
  ASSERT(defn);
  ASSERT(num_args >= 0);
  Do(defn);
  if (IsNativeCall(defn))
    return ir::InvokeNativeInstr::New(defn, num_args);
  return ir::InvokeInstr::New(defn, num_args);
}

auto EffectVisitor::ReturnCall(ir::InvokeInstr* instr) -> bool {
  if (gel::IsPedantic())
    AddInstanceOf(instr, instr->IsInvokeNativeInstr() ? NativeProcedure::GetClass() : Procedure::GetClass());
  ReturnDefinition(instr);
  return true;
}

auto EffectVisitor::ReturnCallTo(ir::Definition* defn, const uword num_args) -> bool {
  const auto invoke = CreateCallFor(defn, num_args);
  if (gel::IsPedantic())
    AddInstanceOf(defn, invoke->IsInvokeNativeInstr() ? NativeProcedure::GetClass() : Procedure::GetClass());
  ReturnDefinition(invoke);
  return true;
}

auto EffectVisitor::ReturnCallTo(Procedure* target, const uword num_args) -> bool {
  ASSERT(target);
  const auto defn = ir::ConstantInstr::New(target);
  return ReturnCallTo(defn, num_args);
}

auto EffectVisitor::VisitCallProcExpr(CallProcExpr* expr) -> bool {
  ASSERT(expr && expr->HasTarget());
  for (auto idx = 1; idx < expr->GetNumberOfChildren(); idx++) {
    const auto arg = expr->GetChildAt(idx);
    ASSERT(arg);
    ValueVisitor for_value(GetOwner());
    LOG_IF(ERROR, !arg->Accept(&for_value)) << "failed to determine value for: " << expr->ToString();
    Append(for_value);
  }
  ValueVisitor for_target(GetOwner());
  if (!expr->GetTarget()->Accept(&for_target)) {
    LOG(ERROR) << "failed to visit target: " << expr->GetTarget()->ToString();
    return false;
  }
  ASSERT(for_target.HasValue());
  Append(for_target);
  return ReturnCallTo(for_target.GetValue(), expr->GetNumberOfArgs());
}

auto EffectVisitor::VisitCaseExpr(expr::CaseExpr* expr) -> bool {
  ASSERT(expr);
  const auto join = JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  for (const auto& clause : expr->GetClauses()) {
    ASSERT(clause);
    EffectVisitor for_clause(GetOwner());
    if (clause && !clause->Accept(&for_clause)) {
      LOG(ERROR) << "failed to visit clause: " << clause->ToString();
      return false;
    }
    for_clause.Add(GotoInstr::New(join));

    ValueVisitor for_test(GetOwner());
    if (!expr->GetKey()->Accept(&for_test)) {
      return false;
    }
    ASSERT(clause->GetKey());
    if (!clause->GetKey()->Accept(&for_test)) {
      LOG(ERROR) << "failed to visit test for cond: " << expr->ToString();
      return false;
    }

    ASSERT(for_clause.GetEntryInstr() != nullptr && for_clause.GetEntryInstr()->IsEntryInstr());
    const auto target = for_clause.GetEntryInstr()->AsEntryInstr();
    const auto cmp = ir::BinaryOpInstr::NewEquals(for_test.GetValue(), for_test.GetValue());  // TODO: fix this
    for_test.Add(cmp);
    const auto branch = ir::BranchInstr::BranchTrue(target, join);
    for_test.Add(branch);
    Append(for_test);
    GetOwner()->GetCurrentBlock()->AddDominated(target);
  }

  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitClauseExpr(expr::ClauseExpr* expr) -> bool {
  ASSERT(expr);

  const auto target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(target);
  Add(target);

  auto remaining = expr->GetNumberOfActions();
  for (const auto& action : expr->GetActions()) {
    ASSERT(action);
    EffectVisitor for_action(GetOwner());
    if (!action->Accept(&for_action)) {
      LOG(ERROR) << "failed to visit action for: " << expr->ToString();
      return false;
    }
    if (--remaining <= 0)
      for_action.AddImplicitReturn();
    AppendFragment(target, for_action);
    SetExitInstr(for_action.GetExitInstr());
  }
  GetOwner()->GetCurrentBlock()->AddDominated(target);
  return true;
}

auto EffectVisitor::VisitWhenExpr(expr::WhenExpr* expr) -> bool {
  ASSERT(expr);
  const auto join = ir::JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  // process conseq
  const auto conseq_target = ir::TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  for (const auto& action : expr->GetActions()) {
    EffectVisitor for_conseq(GetOwner());
    if (!action->Accept(&for_conseq)) {
      LOG(ERROR) << "failed to visit action for: " << expr->ToString();
      return false;
    }
    AppendFragment(conseq_target, for_conseq);
  }
  conseq_target->Append(ir::GotoInstr::New(join));
  GetOwner()->GetCurrentBlock()->AddDominated(conseq_target);

  // process test
  ValueVisitor for_test(GetOwner());
  if (!expr->GetTest()->Accept(&for_test)) {
    LOG(ERROR) << "failed to visit test for cond: " << expr->ToString();
    return false;
  }
  Append(for_test);

  const auto branch = ir::BranchInstr::BranchTrue(conseq_target, join);
  ASSERT(branch);
  Add(branch);
  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitMacroDef(MacroDef* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement @s0cks
  return true;
}

auto EffectVisitor::VisitWhileExpr(expr::WhileExpr* expr) -> bool {  // TODO: clean this up @s0cks
  ASSERT(expr);
  const auto target = ir::TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(target);
  Add(target);

  const auto body_target = ir::TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(body_target);

  const auto join = ir::JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  ValueVisitor for_test(GetOwner());
  if (!expr->GetTest()->Accept(&for_test)) {
    LOG(ERROR) << "failed to visit test for: " << expr->ToString();
    return false;
  }
  AppendFragment(target, for_test);
  target->Append(ir::BranchInstr::BranchTrue(body_target, join));

  EffectVisitor for_body(GetOwner());
  for (const auto& expr : expr->GetBody()) {
    if (!expr->Accept(&for_body)) {
      LOG(ERROR) << "failed to visit action for: " << expr->ToString();
      return false;
    }
  }
  AppendFragment(body_target, for_body);
  body_target->Append(ir::GotoInstr::New(target));

  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(target);
  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitImportExpr(expr::ImportExpr* expr) -> bool {
  ASSERT(expr);
  return true;
}

static inline auto IsLiteralSymbol(expr::LiteralExpr* expr, Symbol* value) -> bool {
  return expr && expr->HasValue() && expr->GetValue()->IsSymbol() && expr->GetValue()->AsSymbol()->Equals(value);
}

static inline auto IsCallSymbol(expr::CallProcExpr* expr, Symbol* value) -> bool {
  ASSERT(expr);
  if (!expr->IsCallProcExpr())
    return false;
  const auto target = expr->AsCallProcExpr()->GetTarget();
  ASSERT(target);
  if (!target->IsLiteralExpr())
    return false;
  return IsLiteralSymbol(target->AsLiteralExpr(), value);
}

template <class N>
static inline auto IsCallNativeSymbol(expr::CallProcExpr* expr) -> bool {
  ASSERT(expr);
  return IsCallSymbol(expr, N::GetNativeSymbol());
}

static inline auto IsInvokePublishSubject(expr::Expression* expr) -> bool {
  if (!expr || !expr->IsCallProcExpr())
    return false;
  return IsCallNativeSymbol<proc::rx_publish_subject>(expr->AsCallProcExpr());
}

static inline auto IsInvokeReplaySubject(expr::Expression* expr) -> bool {
  if (!expr || !expr->IsCallProcExpr())
    return false;
  return IsCallSymbol(expr->AsCallProcExpr(), proc::rx_replay_subject::GetNativeSymbol());
}

auto EffectVisitor::VisitNewExpr(expr::NewExpr* expr) -> bool {
  ASSERT(expr);
  if (expr->IsConstantExpr()) {
    const auto constant = expr->EvalToConstant(GetOwner()->GetScope());
    if (!constant) {
      LOG(ERROR) << "failed to create new constant instance of " << expr->GetTargetClass() << " with args "
                 << expr->GetNumberOfChildren() << " falling back to slow path.";
      ReturnDefinition(ir::NewInstr::New(expr->GetTargetClass(), expr->GetNumberOfChildren()));
      return true;
    }
    ReturnDefinition(ir::ConstantInstr::New(constant));
    return true;
  }

  uint64_t aidx = 0;
  while (IsOpen() && (aidx < expr->GetNumberOfChildren())) {
    const auto arg = expr->GetChildAt(aidx++);
    ASSERT(arg);
    ValueVisitor for_arg(GetOwner());
    if (!arg->Accept(&for_arg)) {
      LOG(FATAL) << "failed to visit arg for rx operator.";
      return false;
    }
    Append(for_arg);
  }
  ReturnDefinition(ir::NewInstr::New(expr->GetTargetClass(), expr->GetNumberOfChildren()));
  return true;
}

auto EffectVisitor::VisitQuotedExpr(expr::QuotedExpr* expr) -> bool {
  ASSERT(expr);
  ReturnDefinition(ir::ConstantInstr::New(expr->Get()));
  return true;
}

auto EffectVisitor::VisitRxOpExpr(expr::RxOpExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);
  return false;
}

static inline auto CreateRxOpTarget(Symbol* symbol, LocalScope* scope) -> ir::Definition* {
  LocalVariable* local = nullptr;
  LOG_IF(FATAL, !scope->Lookup(symbol, &local)) << "failed to find LocalVariable: " << symbol;
  ASSERT(local);
  if (local->HasValue())
    return ir::ConstantInstr::New(local->GetValue());
  return ir::LoadLocalInstr::New(local);
}

auto RxEffectVisitor::VisitRxOpExpr(expr::RxOpExpr* expr) -> bool {
  ASSERT(expr);
  Do(GetObservable());
  uint64_t aidx = 0;
  while (IsOpen() && (aidx < expr->GetNumberOfChildren())) {
    const auto arg = expr->GetChildAt(aidx++);
    ASSERT(arg);
    ValueVisitor for_arg(GetOwner());
    if (!arg->Accept(&for_arg)) {
      LOG(FATAL) << "failed to visit arg for rx operator.";
      return false;
    }
    Append(for_arg);
  }

  const auto call_target = CreateRxOpTarget(expr->GetSymbol(), GetOwner()->GetScope());
  Add(call_target);
  if (IsNativeCall(call_target)) {
    if (IsPedantic())
      AddInstanceOf(call_target, NativeProcedure::GetClass());
    Add(ir::InvokeNativeInstr::New(call_target, expr->GetNumberOfChildren() + 1));
  } else {
    if (gel::IsPedantic())
      AddInstanceOf(call_target, Procedure::GetClass());
    Add(ir::InvokeInstr::New(call_target, expr->GetNumberOfChildren()) + 1);
  }
  return true;
}

static inline auto IsLoadSymbol(ValueVisitor& rhs) -> bool {
  return rhs.HasValue() && rhs.GetValue()->IsLoadLocalInstr();
}

auto EffectVisitor::CreateStoreLoad(LocalVariable* local, ir::Definition* value) -> ir::Definition* {
  ASSERT(local);
  ASSERT(value);
  Add(ir::StoreLocalInstr::New(local, value));
  return ir::LoadLocalInstr::New(local);
}

static inline auto IsObservableSource(LocalScope* scope, expr::Expression* expr) -> bool {
  ASSERT(expr);
  if (expr->IsLiteralExpr() && expr->AsLiteralExpr()->HasValue()) {
    const auto literal = expr->AsLiteralExpr()->GetValue();
    ASSERT(literal);
    if (literal->IsSymbol()) {
      // load symbol
      LocalVariable* local = nullptr;
      if (!scope->Lookup(literal->AsSymbol(), &local)) {
        DLOG(WARNING) << "cannot find local: " << literal->AsSymbol();
        return false;
      }
      return local != nullptr;
    } else if (literal->IsObservable()) {
      return true;
    }
  } else if (expr->IsCastExpr()) {
    return expr->AsCastExpr()->GetTargetType()->Is<Observable>();
  }
  return false;
}

static inline auto IsSubjectSource(LocalScope* scope, expr::Expression* expr) -> bool {
  ASSERT(expr);
  if (expr->IsLiteralExpr() && expr->AsLiteralExpr()->HasValue()) {
    const auto literal = expr->AsLiteralExpr()->GetValue();
    ASSERT(literal);
    if (literal->IsSymbol()) {
      // load symbol
      LocalVariable* local = nullptr;
      if (!scope->Lookup(literal->AsSymbol(), &local)) {
        DLOG(WARNING) << "cannot find value for local: " << literal->AsSymbol();
        return false;
      }
      return local != nullptr;
    } else if (literal->IsSubject()) {
      return true;
    }
  } else if (expr->IsCastExpr()) {
    return expr->AsCastExpr()->GetTargetType()->IsInstance<Subject>();
  }
  return false;
}

auto EffectVisitor::VisitLetRxExpr(expr::LetRxExpr* expr) -> bool {
  ASSERT(expr);
  const auto scope = GetOwner()->PushScope({rx::GetRxScope()});
  ASSERT(scope);
  Symbol* symbol = Symbol::New(".");
  ASSERT(symbol);
  const auto local = LocalVariable::New(scope, symbol);  // TODO: convert to lookup @s0cks
  ASSERT(local);
  LOG_IF(FATAL, !scope->Add(local)) << "failed to create: " << (*local);
  ValueVisitor for_source(GetOwner());
  if (!expr->GetSource()->Accept(&for_source)) {
    LOG(FATAL) << "failed to visit observable.";
    return false;
  }
  Append(for_source);
  if (IsObservableSource(scope, expr->GetSource()) || IsSubjectSource(scope, expr->GetSource())) {
    Add(ir::StoreLocalInstr::New(local, for_source.GetValue()));
  } else {
    Add(ir::StoreLocalInstr::New(local, DoCastTo(for_source.GetValue(), Observable::GetClass())));
  }

  // process body
  uint64_t idx = 0;
  while (IsOpen() && (idx < expr->GetNumberOfChildren())) {
    const auto oper_expr = expr->GetOperatorAt(idx++);
    ASSERT(oper_expr);
    RxEffectVisitor for_effect(GetOwner(), ir::LoadLocalInstr::New(local));
    if (!oper_expr->Accept(&for_effect)) {
      LOG(FATAL) << "failed to visit: " << oper_expr;
      return false;
    }
    Append(for_effect);
    if (idx == expr->GetNumberOfChildren()) {
      ir::Definition* return_value = nullptr;
      if (!oper_expr->IsSubscribe() && !oper_expr->IsComplete()) {
        return_value = ir::LoadLocalInstr::New(local);
      } else {
        return_value = ir::ConstantInstr::New(Null())->AsDefinition();
      }
      ASSERT(return_value);
      ReturnDefinition(return_value);
    }
    if (!IsOpen())
      break;
  }
  GetOwner()->PopScope();
  return true;
}

auto EffectVisitor::VisitBinding(expr::Binding* expr) -> bool {
  ASSERT(expr);
  const auto scope = GetOwner()->GetScope();
  ASSERT(scope);
  const auto local = expr->GetLocal();
  ASSERT(local);
  LOG_IF(FATAL, !scope->Add(local)) << "failed to add " << local << " to scope.";
  ir::Definition* defn = nullptr;
  if (IsInvokePublishSubject(expr->GetValue())) {
    const auto value = PublishSubject::New();
    ASSERT(value);
    defn = ir::ConstantInstr::New(value);
    Add(defn);
  } else if (IsInvokeReplaySubject(expr->GetValue())) {
    const auto value = ReplaySubject::New();
    ASSERT(value);
    defn = ir::ConstantInstr::New(value);
    Add(defn);
  } else {
    ValueVisitor for_value(GetOwner());
    if (!expr->GetValue()->Accept(&for_value)) {
      LOG(FATAL) << "failed to visit value for binding.";
      return false;
    }
    Append(for_value);
    defn = for_value.GetValue();
  }
  ASSERT(defn);
  Add(ir::StoreLocalInstr::New(local, defn));
  return true;
}

/*
 * [ TargetEntryInstr ] - PushFrame()
 *      ...
 * [ TargetEntryInstr ] - PushFrame()
 *      ...
 * [ ReturnInstr ] - PopFrame()
 *      ...
 * [ ReturnInstr] - PopFrame()
 */
auto EffectVisitor::VisitLetExpr(expr::LetExpr* expr) -> bool {
  ASSERT(expr);
  const auto target = ir::TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(target);
  Add(ir::GotoInstr::New(target));
  const auto join = ir::JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);
  const auto new_scope = GetOwner()->PushScope();
  ASSERT(new_scope);
  // process body
  uword idx = 0;
  ir::Definition* return_value = nullptr;
  while (IsOpen() && (idx < expr->GetNumberOfChildren())) {
    const auto child = expr->GetChildAt(idx++);
    ASSERT(child);
    ValueVisitor for_value(GetOwner());
    if (!child->Accept(&for_value))
      break;
    AppendFragment(target, for_value);
    return_value = for_value.GetValue();
    if (!IsOpen())
      break;
  }
  if (!return_value) {
    return_value = ir::ConstantInstr::New(Null());
    target->Append(return_value);
  }
  ASSERT(return_value);
  ReturnDefinition(return_value);
  target->Append(ir::GotoInstr::New(join));
  // TODO: need to pop block
  GetOwner()->PopScope();
  SetExitInstr(join);
  return true;
}

auto EffectVisitor::CreateCastTo(ir::Definition* value, Class* target) -> ir::Definition* {
  ASSERT(value);
  ASSERT(target);
  return ir::CastInstr::New(value, target);
}

auto EffectVisitor::VisitCastExpr(expr::CastExpr* expr) -> bool {
  ASSERT(expr);
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit: " << expr->ToString();
    return false;
  }
  Append(for_value);
  ReturnDefinition(CreateCastTo(for_value.GetValue(), expr->GetTargetType()));
  return true;
}

auto EffectVisitor::VisitBeginExpr(BeginExpr* expr) -> bool {
  ASSERT(expr);
  uint64_t idx = 0;
  while (IsOpen() && (idx < expr->GetNumberOfChildren())) {
    const auto child = expr->GetChildAt(idx++);
    ASSERT(child);
    EffectVisitor vis(GetOwner());
    if (!child->Accept(&vis))
      break;
    Append(vis);
    if (!IsOpen())
      break;
  }
  return true;
}

auto EffectVisitor::VisitCondExpr(CondExpr* expr) -> bool {
  ASSERT(expr);
  const auto alt_target = ir::TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(alt_target);
  const auto join = ir::JoinEntryInstr::New(GetOwner()->GetNextBlockId());

  for (const auto& clause : expr->GetClauses()) {
    // process conseq
    const auto target = ir::TargetEntryInstr::New(GetOwner()->GetNextBlockId());
    ASSERT(target);
    for (const auto& action : clause->GetActions()) {
      ASSERT(action);
      ValueVisitor for_action(GetOwner());
      if (!action->Accept(&for_action)) {
        LOG(ERROR) << "failed to visit conseq for cond: " << expr->ToString();
        return false;
      }
      AppendFragment(target, for_action);
    }
    target->Append(ir::GotoInstr::New(join));
    GetOwner()->GetCurrentBlock()->AddDominated(target);

    ir::BranchInstr* branch = nullptr;
    if (clause->GetKey()->IsBinaryOpExpr()) {
      const auto cond = clause->GetKey()->AsBinaryOpExpr();
      ASSERT(cond);
      if (cond->IsEqualsOp()) {
        // lhs
        ValueVisitor for_left(GetOwner());
        LOG_IF(FATAL, !cond->GetLeft()->Accept(&for_left)) << "failed to visit: " << cond->GetLeft();
        Append(for_left);
        // rhs
        ValueVisitor for_right(GetOwner());
        LOG_IF(FATAL, !cond->GetRight()->Accept(&for_right)) << "failed to visit: " << cond->GetRight();
        Append(for_right);
        branch = ir::BranchInstr::BranchEqual(target, alt_target, join);
      }
    }
    if (!branch) {
      ValueVisitor for_test(GetOwner());
      if (!clause->GetKey()->Accept(&for_test)) {
        LOG(ERROR) << "failed to visit clause for cond: " << expr->ToString();
        return false;
      }
      Append(for_test);
      branch = ir::BranchInstr::BranchTrue(target, alt_target, join);
    }
    ASSERT(branch);
    Add(branch);
  }

  if (expr->HasAlternate()) {  // process alt (else)
    ValueVisitor for_alt(GetOwner());
    if (!expr->GetAlternate()->Accept(&for_alt)) {
      LOG(ERROR) << "failed to visit alternate for cond: " << expr->ToString();
      return false;
    }
    AppendFragment(alt_target, for_alt);
    alt_target->Append(ir::GotoInstr::New(join));
    GetOwner()->GetCurrentBlock()->AddDominated(alt_target);
  }

  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitUnaryExpr(expr::UnaryExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit value for: " << expr->ToString();
    return false;
  }
  Append(for_value);
  switch (expr->GetOp()) {
    case expr::kCar:
    case expr::kCdr:
      if (IsPedantic())
        AddInstanceOf(for_value.GetValue(), Pair::GetClass());
    default:
      ReturnDefinition(ir::UnaryOpInstr::New(expr->GetOp(), for_value.GetValue()));
  }
  return true;
}

auto EffectVisitor::VisitLocalDef(LocalDef* expr) -> bool {
  ASSERT(expr);
  LocalVariable* local = expr->GetLocal();
  ASSERT(local);
  const auto value = expr->GetValue();
  ASSERT(value);
  ValueVisitor for_value(GetOwner());
  if (!value->Accept(&for_value)) {
    LOG(FATAL) << "failed to determine value for: " << expr->ToString();
    return false;
  }
  Append(for_value);
  ASSERT(for_value.HasValue());
  Add(ir::StoreLocalInstr::New(local, for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitListExpr(expr::ListExpr* expr) -> bool {
  ASSERT(expr);
  if (expr->IsConstantExpr()) {
    ReturnDefinition(ir::ConstantInstr::New(expr->EvalToConstant()));
    return true;
  }
  SeqExprIterator<expr::ListExpr> iter(this, expr);
  while (iter.HasNext()) {
    const auto [_, child] = iter.Next();
    ValueVisitor for_value(GetOwner());
    if (!child->Accept(&for_value)) {
      LOG(ERROR) << "failed to visit: " << child;
      return false;
    }
    Append(for_value);
    const auto value = for_value.GetValue();
    ASSERT(value);
  }
  return ReturnCallTo(gel::proc::list::Get(), expr->GetNumberOfChildren());
}

auto EffectVisitor::VisitLiteralExpr(LiteralExpr* p) -> bool {
  ASSERT(p);
  const auto value = p->GetValue();
  ASSERT(value);
  if (value->IsSymbol()) {
    LocalVariable* local = nullptr;
    if (!GetOwner()->GetScope()->Lookup(value->AsSymbol(), &local)) {
      LOG(FATAL) << "failed to find local: " << value->AsSymbol();
    }
    ASSERT(local);
    if (local->HasValue()) {
      ReturnDefinition(ir::ConstantInstr::New(local->GetValue()));
      return true;
    }
    ReturnDefinition(ir::LoadLocalInstr::New(local));
    return true;
  } else {
    ReturnDefinition(ir::ConstantInstr::New(p->GetValue()));
  }
  return true;
}

static inline auto IsConstantSymbol(ir::Definition* defn) -> bool {
  return defn && defn->IsConstantInstr() && defn->AsConstantInstr()->GetValue() &&
         defn->AsConstantInstr()->GetValue()->IsSymbol();
}

static inline auto IsConstantString(ir::Definition* defn) -> bool {
  return defn && defn->IsConstantInstr() && defn->AsConstantInstr()->GetValue() &&
         defn->AsConstantInstr()->GetValue()->IsString();
}

static inline auto GetClassReference(ir::Definition* defn) -> Class* {
  if (IsConstantSymbol(defn)) {
    return Class::FindClass(ToSymbol(defn->AsConstantInstr()->GetValue()));
  } else if (IsConstantString(defn)) {
    return Class::FindClass(ToString(defn->AsConstantInstr()->GetValue()));
  }
  return nullptr;
}

auto EffectVisitor::VisitBinaryOpExpr(BinaryOpExpr* expr) -> bool {
  ASSERT(expr);
  const auto op = expr->GetOp();

  ASSERT(expr->HasLeft());
  ValueVisitor for_left(GetOwner());
  if (!expr->GetLeft()->Accept(&for_left))
    return false;
  Append(for_left);

  ASSERT(expr->HasRight());
  ValueVisitor for_right(GetOwner());
  if (!expr->GetRight()->Accept(&for_right))
    return false;
  Append(for_right);
  ReturnDefinition(ir::BinaryOpInstr::New(op, for_left.GetValue(), for_right.GetValue()));
  return true;
}

auto EffectVisitor::VisitInstanceOfExpr(expr::InstanceOfExpr* expr) -> bool {
  ASSERT(expr);
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  const auto type = Bind(ir::ConstantInstr::New(expr->GetTarget()));
  ASSERT(type);
  ReturnDefinition(ir::BinaryOpInstr::New(BinaryOp::kInstanceOf, for_value.GetValue(), type));
  return true;
}

auto EffectVisitor::VisitThrowExpr(expr::ThrowExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  if (gel::IsPedantic())
    AddInstanceOf(for_value.GetValue(), Error::GetClass());
  Add(ir::ThrowInstr::New(for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitSetExpr(expr::SetExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  LocalVariable* local = expr->GetLocal();
  ASSERT(local);
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit SetExpr value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  ASSERT(for_value.HasValue());
  Add(ir::StoreLocalInstr::New(local, for_value.GetValue()));
  return true;
}

auto FlowGraphBuilder::Build(Lambda* lambda, LocalScope* scope) -> FlowGraph* {
  ASSERT(lambda);
  FlowGraphBuilder builder(scope);
  const auto graph_entry = ir::GraphEntryInstr::New(builder.GetNextBlockId());
  ASSERT(graph_entry);
  builder.SetCurrentBlock(graph_entry);
  const auto target = ir::TargetEntryInstr::New(builder.GetNextBlockId());
  ASSERT(target);
  builder.SetCurrentBlock(target);
  ValueVisitor for_value(&builder);
  if (!for_value.VisitLambda(lambda)) {
    LOG(ERROR) << "failed to visit: " << lambda;
    return nullptr;
  }
  AppendFragment(target, for_value);
  graph_entry->Append(target);
  graph_entry->AddDominated(target);
  return new FlowGraph(graph_entry);
}

auto EffectVisitor::VisitScript(Script* script) -> bool {
  auto index = 0;
  ir::Definition* return_value = nullptr;
  const auto& body = script->GetBody();
  while (IsOpen() && (index < body.size())) {
    const auto expr = body[index++];
    ASSERT(expr);
    ValueVisitor for_value(GetOwner());
    LOG_IF(FATAL, !expr->Accept(&for_value)) << "failed to visit: " << expr->ToString();
    Append(for_value);
    return_value = for_value.GetValue();
    if (!IsOpen()) {
      LOG(WARNING) << "breaking";
      break;
    }
  }
  if (!return_value)
    return_value = Bind(ir::ConstantInstr::New(Null()));
  Add(ir::ReturnInstr::New(return_value));
  return true;
}

auto EffectVisitor::VisitLambda(Lambda* lambda) -> bool {
  const auto scope = GetOwner()->PushScope();
  ASSERT(scope);
  if (lambda->HasScope())
    LOG_IF(FATAL, !scope->Add(lambda->GetScope())) << "failed to add lambda scope to current scope.";
  for (const auto& arg : lambda->GetArgs()) {
    const auto local = LocalVariable::New(scope, Symbol::New(arg.GetName()));
    LOG_IF(FATAL, !scope->Add(local)) << "failed to add " << (*local) << " to current scope";
  }
  auto index = 0;
  const auto& body = lambda->GetBody();
  while (IsOpen() && (index < body.size())) {
    const auto expr = body[index++];
    ASSERT(expr);
    ValueVisitor for_value(GetOwner());
    if (!expr->Accept(&for_value)) {
      LOG(ERROR) << "failed to visit: " << expr->ToString();
      return false;
    }
    Append(for_value);
    if (index == body.size()) {
      auto return_value = for_value.GetValue();
      if (!return_value && !for_value.GetExitInstr()->IsJoinEntryInstr())
        return_value = Bind(ir::ConstantInstr::New(Null()));
      Add(ir::ReturnInstr::New(return_value));
    }
    if (!IsOpen())
      break;
  }
  GetOwner()->PopScope();
  return true;
}

auto FlowGraphBuilder::Build(Script* script, LocalScope* scope) -> FlowGraph* {
  ASSERT(script);
  ASSERT(scope);
  FlowGraphBuilder builder(scope);
  const auto graph_entry = ir::GraphEntryInstr::New(builder.GetNextBlockId());
  ASSERT(graph_entry);
  builder.SetCurrentBlock(graph_entry);
  const auto target = ir::TargetEntryInstr::New(builder.GetNextBlockId());
  ASSERT(target);
  builder.SetCurrentBlock(target);
  ValueVisitor for_effect(&builder);
  if (!for_effect.VisitScript(script)) {
    LOG(ERROR) << "failed to visit: " << script;
    return nullptr;
  }
  AppendFragment(target, for_effect);
  graph_entry->Append(target);
  graph_entry->AddDominated(target);
  return new FlowGraph(graph_entry);
}
}  // namespace gel