#include "scheme/flow_graph_builder.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/lambda.h"
#include "scheme/local.h"
#include "scheme/native_procedure.h"
#include "scheme/natives.h"
#include "scheme/object.h"
#include "scheme/procedure.h"
#include "scheme/script.h"

namespace scm {

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

auto EffectVisitor::VisitEvalExpr(EvalExpr* expr) -> bool {
  ASSERT(expr && expr->HasExpression());
  ValueVisitor for_value(GetOwner());
  if (!expr->GetExpression()->Accept(&for_value)) {
    LOG(ERROR) << "failed to visit EvalExpr expr: " << expr->GetExpression()->ToString();
    return false;
  }
  Append(for_value);
  ReturnDefinition(EvalInstr::New(for_value.GetValue()));
  return true;
}

static inline auto IsNativeCall(instr::Instruction* instr) -> bool {
  ASSERT(instr);
  if (!instr->IsConstantInstr())
    return false;
  const auto target = instr->AsConstantInstr()->GetValue();
  ASSERT(target);
  return target->IsNativeProcedure();
}

void EffectVisitor::AddInstanceOf(instr::Definition* defn, Class* cls) {
  ASSERT(defn);
  return Add(instr::InstanceOfInstr::New(defn, cls));
}

auto EffectVisitor::CreateCallFor(instr::Definition* defn, const uword num_args) -> instr::InvokeInstr* {
  ASSERT(defn);
  ASSERT(num_args >= 0);
  Do(defn);
  if (IsNativeCall(defn))
    return instr::InvokeNativeInstr::New(defn, num_args);
  return instr::InvokeInstr::New(defn, num_args);
}

auto EffectVisitor::ReturnCall(instr::InvokeInstr* instr) -> bool {
#ifndef SCM_RELAXED
  AddInstanceOf(instr, instr->IsInvokeNativeInstr() ? NativeProcedure::GetClass() : Procedure::GetClass());
#endif  // SCM_RELAXED
  ReturnDefinition(instr);
  return true;
}

auto EffectVisitor::ReturnCall(Procedure* target, const uword num_args) -> bool {
  ASSERT(target);
  return ReturnCall(CreateCallFor(instr::ConstantInstr::New(target), num_args));
}

auto EffectVisitor::ReturnCall(Symbol* symbol, const uword num_args) -> bool {
  return ReturnCall(CreateCallFor(instr::LoadVariableInstr::New(symbol), num_args));
}

auto EffectVisitor::VisitCallProcExpr(CallProcExpr* expr) -> bool {
  ASSERT(expr);
  ValueVisitor for_target(GetOwner());
  {
    const auto target = expr->GetTarget();
    ASSERT(target);
    if (!target->Accept(&for_target)) {
      LOG(ERROR) << "failed to visit target: " << expr->GetTarget()->ToString();
      return false;
    }
  }
  const auto target = for_target.GetValue();
  ASSERT(target);
  for (auto idx = 1; idx < expr->GetNumberOfChildren(); idx++) {
    const auto arg = expr->GetChildAt(idx);
    ASSERT(arg);
    ValueVisitor for_value(GetOwner());
    LOG_IF(ERROR, !arg->Accept(&for_value)) << "failed to determine value for: " << expr->ToString();
    Append(for_value);
  }
  Append(for_target);

  if (IsNativeCall(target)) {
    Add(InstanceOfInstr::New(target, NativeProcedure::GetClass()));
    ReturnDefinition(InvokeNativeInstr::New(for_target.GetValue(), expr->GetNumberOfArgs()));
    return true;
  }

  Add(InstanceOfInstr::New(target, Procedure::GetClass()));
  ReturnDefinition(InvokeInstr::New(target, expr->GetNumberOfArgs()));
  return true;
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
    const auto cmp = instr::BinaryOpInstr::NewEquals(for_test.GetValue(), for_test.GetValue());  // TODO: fix this
    for_test.Add(cmp);
    const auto branch = BranchInstr::New(cmp, target, join);
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
  const auto join = JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  // process conseq
  const auto conseq_target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  for (const auto& action : expr->GetActions()) {
    EffectVisitor for_conseq(GetOwner());
    if (!action->Accept(&for_conseq)) {
      LOG(ERROR) << "failed to visit action for: " << expr->ToString();
      return false;
    }
    AppendFragment(conseq_target, for_conseq);
  }
  conseq_target->Append(GotoInstr::New(join));
  GetOwner()->GetCurrentBlock()->AddDominated(conseq_target);

  // process test
  ValueVisitor for_test(GetOwner());
  if (!expr->GetTest()->Accept(&for_test)) {
    LOG(ERROR) << "failed to visit test for cond: " << expr->ToString();
    return false;
  }
  Append(for_test);

  const auto branch = BranchInstr::New(for_test.GetValue(), conseq_target, join);
  ASSERT(branch);
  Add(branch);
  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitMacroDef(MacroDef* expr) -> bool {
  ASSERT(expr);
  // TODO:
  //  MacroCompiler compiler;
  //  const auto macro = compiler.CompileMacro(expr);
  //  ASSERT(macro);
  // const auto scope = GetOwner()->GetScope();
  // ASSERT(scope);
  // if (!scope->Add(macro->GetSymbol(), macro)) {
  //   throw Exception(
  //       fmt::format("cannot define Macro symbol `{}` for value: `{}`", macro->GetSymbol()->ToString(), macro->ToString()));
  // }
  return true;
}

auto EffectVisitor::VisitWhileExpr(expr::WhileExpr* expr) -> bool {  // TODO: clean this up
  ASSERT(expr);
  const auto target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(target);
  Add(target);

  const auto body_target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(body_target);

  const auto join = JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  ValueVisitor for_test(GetOwner());
  if (!expr->GetTest()->Accept(&for_test)) {
    LOG(ERROR) << "failed to visit test for: " << expr->ToString();
    return false;
  }
  AppendFragment(target, for_test);
  target->Append(BranchInstr::New(for_test.GetValue(), body_target, join));

  EffectVisitor for_body(GetOwner());
  for (const auto& expr : expr->GetBody()) {
    if (!expr->Accept(&for_body)) {
      LOG(ERROR) << "failed to visit action for: " << expr->ToString();
      return false;
    }
  }
  AppendFragment(body_target, for_body);
  body_target->Append(instr::GotoInstr::New(target));

  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(target);
  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitImportDef(expr::ImportDef* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto EffectVisitor::VisitBinding(expr::Binding* expr) -> bool {
  ASSERT(expr);
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit value for binding.";
    return false;
  }
  Append(for_value);
  Add(instr::StoreVariableInstr::New(symbol, for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitQuotedExpr(expr::QuotedExpr* expr) -> bool {
  ASSERT(expr);
  ReturnDefinition(ConstantInstr::New(expr->Get()));
  return true;
}

auto EffectVisitor::VisitRxOpExpr(expr::RxOpExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);
  return false;
}

static inline auto ResolveRxOp(Symbol* symbol, LocalScope* current_scope) -> Procedure* {
  ASSERT(symbol);
  ASSERT(current_scope);
  LocalVariable* local = nullptr;
  // try rx-scope
  {
    const auto rx_scope = rx::GetRxScope();
    ASSERT(rx_scope);
    if (rx_scope->Lookup(symbol, &local)) {
      if (local->HasValue() && local->GetValue()->IsProcedure())
        return local->GetValue()->AsProcedure();
    }
    DLOG(WARNING) << "failed to find rx target `" << symbol << " in scope: " << rx_scope->ToString();
  }
  if (current_scope->Lookup(symbol, &local)) {
    if (local->HasValue() && local->GetValue()->IsProcedure())
      return local->GetValue()->AsProcedure();
  }
  return nullptr;
}

static inline auto CreateRxOpTarget(Symbol* symbol, LocalScope* scope) -> instr::Definition* {
  const auto procedure = ResolveRxOp(symbol, scope);
  if (procedure)
    return instr::ConstantInstr::New(procedure);
  return instr::LoadVariableInstr::New(symbol);
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
    Add(instr::InvokeNativeInstr::New(call_target, expr->GetNumberOfChildren() + 1));
  } else {
    Add(instr::InstanceOfInstr::New(call_target, Procedure::GetClass()));
    Add(instr::InvokeInstr::New(call_target, expr->GetNumberOfChildren()) + 1);
  }
  return true;
}

static inline auto IsLoadSymbol(ValueVisitor& rhs) -> bool {
  return rhs.HasValue() && rhs.GetValue()->IsLoadVariableInstr();
}

auto EffectVisitor::CreateStoreLoad(Symbol* symbol, instr::Definition* value) -> instr::Definition* {
  ASSERT(value);
  Add(instr::StoreVariableInstr::New(symbol, value));
  return instr::LoadVariableInstr::New(symbol);
}

static inline auto IsConstantType(instr::Definition* value, Class* expected) -> bool {
  ASSERT(value);
  if (!value->IsConstantInstr())
    return false;
  const auto constant = value->AsConstantInstr();
  ASSERT(constant);
  return constant->GetValue()->GetType()->Equals(expected);
}

static inline auto IsConstantInstanceOf(instr::Definition* value, Class* expected) -> bool {
  if (!value || !value->IsConstantInstr())
    return false;
  const auto constant = value->AsConstantInstr();
  ASSERT(constant);
  return constant->GetValue()->GetType()->IsInstanceOf(expected);
}

static inline auto IsConstantObservable(instr::Definition* defn) -> bool {
  return IsConstantType(defn, Observable::GetClass());
}

auto EffectVisitor::VisitLetRxExpr(expr::LetRxExpr* expr) -> bool {
  ASSERT(expr);
  const auto symbol = Symbol::New("observable");
  const auto scope = LocalScope::New(GetOwner()->GetScope());
  ASSERT(scope);
  const auto local = LocalVariable::New(scope, symbol);
  ASSERT(local);
  LOG_IF(FATAL, !scope->Add(local)) << "failed to create: " << (*local);

  ValueVisitor for_observable(GetOwner());
  if (!expr->GetObservable()->Accept(&for_observable)) {
    LOG(FATAL) << "failed to visit observable.";
    return false;
  }
  Append(for_observable);
  const auto observable = IsConstantObservable(for_observable.GetValue())
                            ? for_observable.GetValue()
                            : DoCastTo(for_observable.GetValue(), Observable::GetClass());
  Add(instr::StoreVariableInstr::New(symbol, observable));

  // process body
  uint64_t idx = 0;
  while (IsOpen() && (idx < expr->GetNumberOfChildren())) {
    const auto oper_expr = expr->GetOperatorAt(idx++);
    ASSERT(oper_expr);
    RxEffectVisitor for_effect(GetOwner(), instr::LoadVariableInstr::New(symbol));
    if (!oper_expr->Accept(&for_effect)) {
      LOG(FATAL) << "failed to visit: " << oper_expr;
      return false;
    }
    Append(for_effect);
    if (idx == expr->GetNumberOfChildren()) {
      const auto return_value = !oper_expr->IsSubscribe() ? instr::LoadVariableInstr::New(symbol)->AsDefinition()
                                                          : instr::ConstantInstr::New(Null())->AsDefinition();
      ASSERT(return_value);
      ReturnDefinition(return_value);
    }
    if (!IsOpen())
      break;
  }
  return true;
}

auto EffectVisitor::VisitLetExpr(expr::LetExpr* expr) -> bool {
  ASSERT(expr);
  // process body
  uint64_t idx = 0;
  while (IsOpen() && (idx < expr->GetNumberOfChildren())) {
    const auto child = expr->GetChildAt(idx++);
    ASSERT(child);
    EffectVisitor for_effect(GetOwner());
    if (!child->Accept(&for_effect))
      break;
    Append(for_effect);
    if (!IsOpen())
      break;
  }
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
  const auto join = JoinEntryInstr::New(GetOwner()->GetNextBlockId());

  for (const auto& clause : expr->GetClauses()) {
    // process conseq
    const auto target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
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
    target->Append(GotoInstr::New(join));
    GetOwner()->GetCurrentBlock()->AddDominated(target);

    ValueVisitor for_test(GetOwner());
    if (!clause->GetKey()->Accept(&for_test)) {
      LOG(ERROR) << "failed to visit clause for cond: " << expr->ToString();
      return false;
    }
    Append(for_test);
    const auto branch = BranchInstr::New(for_test.GetValue(), target, join);
    ASSERT(branch);
    Add(branch);
  }

  if (expr->HasAlternate()) {  // process alt (else)
    const auto target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
    ASSERT(target);
    Add(target);
    ValueVisitor for_alt(GetOwner());
    if (!expr->GetAlternate()->Accept(&for_alt)) {
      LOG(ERROR) << "failed to visit alternate for cond: " << expr->ToString();
      return false;
    }
    AppendFragment(target, for_alt);
    target->Append(GotoInstr::New(join));
    GetOwner()->GetCurrentBlock()->AddDominated(target);
  }

  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitLambdaExpr(LambdaExpr* expr) -> bool {
  const auto lambda = Lambda::New(expr->GetArgs(), expr->GetBody());
  ASSERT(lambda);
  ReturnDefinition(ConstantInstr::New(lambda));
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
      Add(instr::InstanceOfInstr::New(for_value.GetValue(), Pair::GetClass()));
    default:
      ReturnDefinition(instr::UnaryOpInstr::New(expr->GetOp(), for_value.GetValue()));
  }
  return true;
}

auto EffectVisitor::VisitLocalDef(LocalDef* expr) -> bool {
  ASSERT(expr);
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  // process value
  const auto value = expr->GetValue();
  ASSERT(value);
  ValueVisitor for_value(GetOwner());
  if (!value->Accept(&for_value)) {
    LOG(FATAL) << "failed to determine value for: " << expr->ToString();
    return false;
  }
  Append(for_value);
  Add(StoreVariableInstr::New(symbol, for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitListExpr(expr::ListExpr* expr) -> bool {
  ASSERT(expr);
  if (expr->IsConstantExpr()) {
    ReturnDefinition(instr::ConstantInstr::New(expr->EvalToConstant()));
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
  return ReturnCall(scm::proc::list::Get(), expr->GetNumberOfChildren());
}

auto EffectVisitor::VisitLiteralExpr(LiteralExpr* p) -> bool {
  ASSERT(p);
  const auto value = p->GetValue();
  ASSERT(value);
  if (value->IsSymbol()) {
    LocalVariable* local = nullptr;
    if (!GetOwner()->GetScope()->Lookup(value->AsSymbol(), &local)) {
      ReturnDefinition(instr::LoadVariableInstr::New(value->AsSymbol()));
      return true;
    }
    ASSERT(local);
    if (local->HasValue() && local->GetValue()->IsNativeProcedure()) {
      ReturnDefinition(ConstantInstr::New(local->GetValue()));
      return true;
    }
    ReturnDefinition(instr::LoadVariableInstr::New(value->AsSymbol()));
    return true;
  } else {
    ReturnDefinition(ConstantInstr::New(p->GetValue()));
  }
  return true;
}

static inline auto IsConstantSymbol(instr::Definition* defn) -> bool {
  return defn && defn->IsConstantInstr() && defn->AsConstantInstr()->GetValue() &&
         defn->AsConstantInstr()->GetValue()->IsSymbol();
}

static inline auto IsConstantString(instr::Definition* defn) -> bool {
  return defn && defn->IsConstantInstr() && defn->AsConstantInstr()->GetValue() &&
         defn->AsConstantInstr()->GetValue()->IsString();
}

static inline auto GetClassReference(instr::Definition* defn) -> Class* {
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
  ReturnDefinition(BinaryOpInstr::New(op, for_left.GetValue(), for_right.GetValue()));
  return true;
}

auto EffectVisitor::VisitInstanceOfExpr(expr::InstanceOfExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto EffectVisitor::VisitThrowExpr(expr::ThrowExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  Add(InstanceOfInstr::New(for_value.GetValue(), String::GetClass()));
  Add(ThrowInstr::New(for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitSetExpr(expr::SetExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit SetExpr value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  Add(StoreVariableInstr::New(expr->GetSymbol(), for_value.GetValue()));
  return true;
}

auto FlowGraphBuilder::Build(Expression* expr, LocalScope* scope) -> FlowGraph* {
  ASSERT(expr);
  ASSERT(scope);
  FlowGraphBuilder builder(scope);
  const auto graph_entry = GraphEntryInstr::New(builder.GetNextBlockId());
  ASSERT(graph_entry);
  builder.SetCurrentBlock(graph_entry);
  const auto target = TargetEntryInstr::New(builder.GetNextBlockId());
  ASSERT(target);
  builder.SetCurrentBlock(target);
  EffectVisitor for_effect(&builder);
  if (!expr->Accept(&for_effect)) {
    LOG(ERROR) << "failed to visit: " << expr->ToString();
    return nullptr;  // TODO: free entry
  }
  const auto exit = for_effect.GetExitInstr();
  ASSERT(exit);
  if (exit && !exit->IsReturnInstr()) {
    const auto new_exit = exit->IsDefinition() ? instr::ReturnInstr::New(exit->AsDefinition()) : instr::ReturnInstr::New();
    ASSERT(new_exit);
    for_effect.Add(new_exit);
  }
  ASSERT(for_effect.GetExitInstr() && for_effect.GetExitInstr()->IsReturnInstr());
  AppendFragment(target, for_effect);
  graph_entry->Append(target);
  graph_entry->AddDominated(target);
  return new FlowGraph(graph_entry);
}

auto EffectVisitor::VisitScript(Script* script) -> bool {
  auto index = 0;
  const auto& body = script->GetBody();
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
      const auto return_value = for_value.HasValue() ? for_value.GetValue() : instr::ConstantInstr::New(Null());
      ASSERT(return_value);
      Do(return_value);
      AddReturnExit(return_value);
    }
    if (!IsOpen())
      break;
  }
  return true;
}

auto FlowGraphBuilder::Build(Script* script, LocalScope* scope) -> FlowGraph* {
  ASSERT(script);
  ASSERT(scope);
  FlowGraphBuilder builder(scope);
  const auto graph_entry = GraphEntryInstr::New(builder.GetNextBlockId());
  ASSERT(graph_entry);
  builder.SetCurrentBlock(graph_entry);
  const auto target = TargetEntryInstr::New(builder.GetNextBlockId());
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
}  // namespace scm