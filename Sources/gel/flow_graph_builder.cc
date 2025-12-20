#include "flow_graph_builder.h"

#include <cstdint>
#include <fmt/format.h>
#include <glog/logging.h>
#include <utility>

#include "common.h"
#include "expr/expression.h"
#include "expr/exprs.h"
#include "flags.h"
#include "flow_graph.h"
#include "instruction.h"
#include "iterator.h"
#include "lambda.h"
#include "local.h"
#include "local_scope.h"
#include "map.h"
#include "natives.h"
#include "object.h"
#include "platform.h"
#include "rx.h"
#include "script.h"
#include "types.h"

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
  return target->IsNativeFn();
}

static inline auto IsLambdaCall(ir::Instruction* instr) -> bool {
  ASSERT(instr);
  if (!instr->IsConstantInstr())
    return false;
  const auto target = instr->AsConstantInstr()->GetValue();
  ASSERT(target);
  return target->IsLambda();
}

void EffectVisitor::AddInstanceOf(ir::Definition* defn, Class* cls) {
  ASSERT(defn);
  return Add(ir::InstanceOfInstr::New(defn, cls));
}

auto EffectVisitor::CreateCallFor(ir::Definition* defn, const uword num_args) -> ir::Definition* {
  ASSERT(defn);
  ASSERT(num_args >= 0);
  if (IsNativeCall(defn)) {
    const auto native = defn->AsConstantInstr()->GetValue()->AsNativeFn();
    ASSERT(native);
    return ir::InvokeNativeInstr::New(defn, num_args);
  } else if (IsLambdaCall(defn)) {
    const auto lambda = defn->AsConstantInstr()->GetValue()->AsLambda();
    ASSERT(lambda);
    return ir::InvokeInstr::New(defn, num_args);
  }
  Do(defn);
  if (defn->IsConstantInstr() && defn->AsConstantInstr()->IsConstantSymbol()) {
    if (gel::IsPedantic())
      AddInstanceOf(defn, Symbol::GetClass());
    const auto lookup = Bind(ir::LookupInstr::New(defn));
    ASSERT(lookup);
    return ir::InvokeDynamicInstr::New(lookup, num_args);
  }
  return ir::InvokeDynamicInstr::New(defn, num_args);
}

auto EffectVisitor::ReturnCall(ir::InvokeInstr* instr) -> bool {
  if (gel::IsPedantic() && !instr->IsInvokeNativeInstr())
    AddInstanceOf(instr, Fn::GetClass());
  ReturnDefinition(instr);
  return true;
}

auto EffectVisitor::ReturnCallTo(ir::Definition* defn, const uword num_args) -> bool {
  const auto invoke = CreateCallFor(defn, num_args);
  ASSERT(invoke);
  ReturnDefinition(invoke);
  return true;
}

auto EffectVisitor::ReturnCallTo(Fn* target, const uword num_args) -> bool {
  ASSERT(target);
  const auto defn = ir::ConstantInstr::New(target);
  ASSERT(defn);
  return ReturnCallTo(defn, num_args);
}

auto EffectVisitor::VisitInvokeInstanceExpr(InvokeInstanceExpr* expr) -> bool {
  ASSERT(expr);
  if (expr->GetInstance()->IsLiteralExpr() && expr->GetInstance()->AsLiteralExpr()->IsLiteralSymbol()) {
    const auto symbol = expr->GetInstance()->AsLiteralExpr()->GetValue()->AsSymbol();
    LocalVariable* local = nullptr;
    if (GetOwner()->GetScope()->Lookup(symbol, &local)) {
      Add(ir::LoadLocalInstr::New(local));
    } else {
      goto default_for_instance;  // NOLINT(cppcoreguidelines-avoid-goto)
    }
  } else {
  default_for_instance:
    ValueVisitor for_instance(GetOwner());
    if (!expr->GetInstance()->Accept(for_instance)) {
      LOG(ERROR) << "failed to visit: " << expr->GetInstance()->ToString();
      return false;
    }
    Append(for_instance);
  }

  for (auto idx = 1; idx < expr->GetNumberOfArgs(); idx++) {
    const auto arg = expr->GetArgAt(idx);
    ASSERT(arg);
    ValueVisitor for_value(GetOwner());
    LOG_IF(ERROR, !arg->Accept(for_value)) << "failed to determine value for: " << expr->ToString();
    Append(for_value);
  }
  return ReturnCallTo(expr->GetTarget(), expr->GetNumberOfArgs());
}

auto EffectVisitor::VisitInvokeNativeExpr(InvokeNativeExpr* expr) -> bool {
  ASSERT(expr);
  for (auto idx = 0; idx < expr->GetNumberOfChildren(); idx++) {
    const auto arg = expr->GetChildAt(idx);
    ASSERT(arg);
    ValueVisitor for_value(GetOwner());
    LOG_IF(ERROR, !arg->Accept(for_value)) << "failed to determine value for: " << expr->ToString();
    Append(for_value);
  }
  return ReturnCallTo(expr->GetTarget(), expr->GetNumberOfArgs());
}

auto EffectVisitor::VisitInvokeExpr(InvokeExpr* expr) -> bool {
  ASSERT(expr);
  for (auto idx = 1; idx < expr->GetNumberOfChildren(); idx++) {
    const auto arg = expr->GetChildAt(idx);
    ASSERT(arg);
    ValueVisitor for_value(GetOwner());
    LOG_IF(ERROR, !arg->Accept(for_value)) << "failed to determine value for: " << expr->ToString();
    Append(for_value);
  }
  ValueVisitor for_target(GetOwner());
  if (!for_target(expr->GetTarget())) {
    LOG(ERROR) << "failed to visit target: " << expr->GetTarget()->ToString();
    return false;
  }
  ASSERT(for_target.HasValue());
  return ReturnCallTo(for_target.GetValue(), expr->GetNumberOfArgs());
}

auto EffectVisitor::VisitClauseExpr(expr::ClauseExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

/*

- cond
- branch if-false -> join
- loop_body:
-   ....
-   cond
-   branch if-true -> loop_body
- join:
-   ....

 */
auto EffectVisitor::VisitWhileExpr(expr::WhileExpr* expr) -> bool {  // TODO: clean this up @s0cks
  ASSERT(expr);
  const auto body = ir::TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(body);
  const auto join = ir::JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  EffectVisitor for_body(GetOwner());
  if (!expr->GetBody()->Accept(for_body)) {
    LOG(ERROR) << "failed to visit while-expr body.";
    return false;
  }
  AppendFragment(body, for_body);
  {
    ValueVisitor for_test(GetOwner());
    if (!expr->GetTest()->Accept(for_test)) {
      LOG(ERROR) << "failed to visit test for: " << expr->ToString();
      return false;
    }
    AppendFragment(body, for_test);
    body->Append(ir::BranchInstr::BranchTrue(body, join));
  }

  {
    ValueVisitor for_test(GetOwner());
    if (!expr->GetTest()->Accept(for_test)) {
      LOG(ERROR) << "failed to visit test for: " << expr->ToString();
      return false;
    }
    Append(for_test);
    Add(ir::BranchInstr::BranchFalse(join, body, join));
  }

  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(body);
  return true;
}

auto EffectVisitor::VisitForeachExpr(expr::ForeachExpr* expr) -> bool {
  ASSERT(expr);
  const auto body = ir::TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(body);
  const auto join = ir::JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  const auto binding = expr->GetBindingAt(0);
  ValueVisitor for_binding(GetOwner());
  if (!for_binding(binding)) {
    LOG(ERROR) << "failed to visit foreach-expr binding: " << binding->ToString();
    return false;
  }
  Append(for_binding);

  const auto scope = GetOwner()->GetScope();
  LocalVariable* it_local = nullptr;
  if (!scope->Lookup("$", &it_local)) {
    LOG(ERROR) << "failed to find it_local in current scope:";
    _PRINT_SCOPE_AT_LEVEL(google::ERROR, scope, true);
    return false;
  }

  Add(ir::StoreLocalInstr::New(it_local, Bind(ir::NewInstr::New(Iterator::GetClass(), 1))));
  Add(ir::LoadLocalInstr::New(it_local));
  Add(ir::InvokeNativeInstr::New(ir::ConstantInstr::New(proc::iter_has_next::Get()->GetNative()), 1));
  Add(ir::BranchInstr::BranchTrue(body, join, join));

  EffectVisitor for_body(GetOwner());
  body->Append(ir::LoadLocalInstr::New(it_local));
  const auto next = ir::InvokeNativeInstr::New(ir::ConstantInstr::New(proc::iter_next::Get()->GetNative()), 1);
  body->Append(next);
  body->Append(ir::StoreLocalInstr::New(binding->GetLocal(), next));

  if (!expr->GetBody()->Accept(for_body)) {
    LOG(ERROR) << "failed to visit while-expr body.";
    return false;
  }

  AppendFragment(body, for_body);

  body->Append(ir::LoadLocalInstr::New(it_local));
  body->Append(ir::InvokeNativeInstr::New(ir::ConstantInstr::New(proc::iter_has_next::Get()->GetNative()), 1));
  body->Append(ir::BranchInstr::BranchTrue(body, join, join));
  body->Append(ir::GotoInstr::New(join));

  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(body);
  return true;
}

auto EffectVisitor::VisitImportExpr(expr::ImportExpr* expr) -> bool {
  ASSERT(expr);
  return true;
}

auto EffectVisitor::VisitInvokeMacroExpr(expr::InvokeMacroExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);
  return false;
}

auto EffectVisitor::VisitNewMapExpr(expr::NewMapExpr* expr) -> bool {
  ASSERT(expr);
  for (const auto& e : expr->data()) {
    ASSERT(e.first && e.second);
    Do(ir::ConstantInstr::New(e.first));

    ValueVisitor for_value(GetOwner());
    if (!e.second->Accept(for_value)) {
      LOG(FATAL) << "failed to visit map entry: " << e.first << " := " << e.second;
      return false;
    }
    Append(for_value);
  }
  const auto defn = Bind(ir::NewInstr::New(Map::GetClass(), expr->GetNumberOfChildren() * 2));
  ASSERT(defn);
  ReturnDefinition(defn);
  return true;
}

static inline auto IsLiteralSymbol(expr::LiteralExpr* expr, Symbol* value) -> bool {
  return expr && expr->HasValue() && expr->GetValue()->IsSymbol() && expr->GetValue()->AsSymbol()->Equals(value);
}

static inline auto IsCallSymbol(expr::InvokeExpr* expr, Symbol* value) -> bool {
  ASSERT(expr);
  if (!expr->IsInvokeExpr())
    return false;
  const auto target = expr->AsInvokeExpr()->GetTarget();
  ASSERT(target);
  if (!target->IsLiteralExpr())
    return false;
  return IsLiteralSymbol(target->AsLiteralExpr(), value);
}

template <class N>
static inline auto IsCallNativeSymbol(expr::InvokeExpr* expr) -> bool {
  ASSERT(expr);
  return IsCallSymbol(expr, N::GetNativeSymbol());
}

static inline auto IsInvokePublishSubject(expr::Expression* expr) -> bool {
  if (!expr || !expr->IsInvokeExpr())
    return false;
  return IsCallNativeSymbol<proc::rx_publish_subject>(expr->AsInvokeExpr());
}

static inline auto IsInvokeReplaySubject(expr::Expression* expr) -> bool {
  if (!expr || !expr->IsInvokeExpr())
    return false;
  return IsCallSymbol(expr->AsInvokeExpr(), proc::rx_replay_subject::GetNativeSymbol());
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
    if (!arg->Accept(for_arg)) {
      LOG(FATAL) << "failed to visit arg for rx operator.";
      return false;
    }
    Append(for_arg);
  }
  ReturnDefinition(ir::NewInstr::New(expr->GetTargetClass(), expr->GetNumberOfChildren()));
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

auto ValueVisitor::VisitBindingExpr(expr::BindingExpr* expr) -> bool {
  ASSERT(expr);
  ValueVisitor for_value(GetOwner());
  if (!for_value(expr->GetValue())) {
    LOG(ERROR) << "failed to visit binding-expr value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  if (for_value.HasValue())
    ReturnValue(for_value.GetValue());
  return true;
}

auto EffectVisitor::VisitBindingExpr(expr::BindingExpr* expr) -> bool {
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
    if (!expr->GetValue()->Accept(for_value)) {
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

auto EffectVisitor::VisitLetExpr(expr::LetExpr* expr) -> bool {
  ASSERT(expr);
  const auto target = ir::LetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(target);
  Add(ir::GotoInstr::New(target));
  const auto join = ir::JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  for (auto idx = 0; idx < expr->GetNumberOfBindings(); idx++) {
    const auto binding = expr->GetBindingAt(idx);
    DLOG(INFO) << "visiting: " << binding->ToString();
    ValueVisitor for_value(GetOwner());
    if (!binding->GetValue()->Accept(for_value)) {
      LOG(ERROR) << "failed to visit let-expr binding: " << binding->ToString();
      return false;
    }
    AppendFragment(target, for_value);
    target->Append(ir::StoreLocalInstr::New(binding->GetLocal(), for_value.GetValue()));
  }

  EffectVisitor for_body(GetOwner());
  if (!expr->GetBody()->Accept(for_body)) {
    LOG(ERROR) << "failed to visit: " << expr->GetBody()->ToString();
    return false;
  }
  AppendFragment(target, for_body);

  target->Append(ir::GotoInstr::New(join));
  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(target);
  return true;
}

auto EffectVisitor::CreateCastTo(ir::Definition* value, Class* target) -> ir::Definition* {
  ASSERT(value);
  ASSERT(target);
  return ir::CastInstr::New(value, target);
}

auto EffectVisitor::VisitDoExpr(DoExpr* expr) -> bool {
  ASSERT(expr);
  EffectVisitor for_body(GetOwner());
  if (!expr->GetBody()->Accept(for_body)) {
    LOG(ERROR) << "failed to visit do-expr body: " << expr->GetBody()->ToString();
    return false;
  }
  Append(for_body);
  return true;
}

auto CondClauseEffectVisitor::VisitClauseExpr(expr::ClauseExpr* expr) -> bool {
  ASSERT(expr);
  EffectVisitor for_body(GetOwner());
  if (!for_body(expr->GetBody())) {
    LOG(ERROR) << "failed to visit clause-expr body: " << expr->GetBody()->ToString();
    return false;
  }
  Append(for_body);
  Add(ir::GotoInstr::New(GetJoin()));
  return true;
}

auto EffectVisitor::VisitCondExpr(CondExpr* expr) -> bool {
  ASSERT(expr);
  const auto join = ir::JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);
  for (auto idx = 0; idx < expr->GetNumberOfClauses(); idx++) {
    const auto clause = expr->GetClauseAt(idx);
    ASSERT(clause);
    const auto target = ir::TargetEntryInstr::New(GetOwner()->GetNextBlockId());
    ASSERT(target);
    const auto clause_join = ir::JoinEntryInstr::New(GetOwner()->GetNextBlockId());
    ASSERT(clause_join);

    const auto test = clause->GetKey();
    ValueVisitor for_test(GetOwner());
    if (!for_test(test)) {
      LOG(ERROR) << "failed to visit clause-expr test: " << test->ToString();
      return false;
    }
    Append(for_test);
    Add(ir::BranchInstr::BranchTrue(target, clause_join, clause_join));

    CondClauseEffectVisitor for_clause(GetOwner(), target, join);
    if (!clause->Accept(for_clause)) {
      LOG(ERROR) << "failed to visit clause-expr: " << clause->ToString();
      return false;
    }
    AppendFragment(target, for_clause);

    SetExitInstr(clause_join);
    GetOwner()->GetCurrentBlock()->AddDominated(target);
  }

  if (expr->HasAlternate()) {  // process alt (else)
    ValueVisitor for_alt(GetOwner());
    if (!expr->GetAlternate()->Accept(for_alt)) {
      LOG(ERROR) << "failed to visit alternate for cond: " << expr->ToString();
      return false;
    }
    Append(for_alt);
    Add(ir::GotoInstr::New(join));
  }

  SetExitInstr(join);
  return true;
}

auto EffectVisitor::VisitUnaryOpExpr(expr::UnaryOpExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(for_value)) {
    LOG(FATAL) << "failed to visit value for: " << expr->ToString();
    return false;
  }
  Append(for_value);
  switch (expr->GetOp()) {
    case UnaryOp::kCar:
    case UnaryOp::kCdr:
      if (gel::IsPedantic())
        AddInstanceOf(for_value.GetValue(), Pair::GetClass());
    default:
      ReturnDefinition(ir::UnaryOpInstr::New(expr->GetOp(), for_value.GetValue()));
  }
  return true;
}

auto EffectVisitor::VisitLiteralExpr(LiteralExpr* p) -> bool {
  ASSERT(p);
  const auto value = p->GetValue();
  ASSERT(value);
  if (value->IsSymbol()) {
    LocalVariable* local = nullptr;
    if (!GetOwner()->GetScope()->Lookup(value->AsSymbol(), &local)) {
      ReturnDefinition(ir::ConstantInstr::New(value->AsSymbol()));
      return true;
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
  if (!expr->GetLeft()->Accept(for_left))
    return false;
  Append(for_left);

  ASSERT(expr->HasRight());
  ValueVisitor for_right(GetOwner());
  if (!expr->GetRight()->Accept(for_right))
    return false;
  Append(for_right);
  ReturnDefinition(ir::BinaryOpInstr::New(op, for_left.GetValue(), for_right.GetValue()));
  return true;
}

auto EffectVisitor::VisitLoadFieldExpr(expr::LoadFieldExpr* expr) -> bool {
  ASSERT(expr);
  const auto field = expr->GetField();
  ASSERT(field);
  ValueVisitor for_instance(GetOwner());
  if (!expr->GetInstance()->Accept(for_instance)) {
    LOG(FATAL) << "failed to visit: " << expr->GetInstance();
  }
  Append(for_instance);
  ReturnDefinition(ir::LoadFieldInstr::New(for_instance.GetValue(), field));
  return true;
}

auto EffectVisitor::VisitLoadInstanceMethodExpr(expr::LoadInstanceMethodExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto EffectVisitor::VisitThrowExpr(expr::ThrowExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(for_value)) {
    LOG(FATAL) << "failed to visit value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  AddThrow(for_value.GetValue());
  return true;
}

auto EffectVisitor::VisitStoreLocalExpr(expr::StoreLocalExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  LocalVariable* local = expr->GetLocal();
  ASSERT(local);
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(for_value)) {
    LOG(FATAL) << "failed to visit SetExpr value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  ASSERT(for_value.HasValue());
  Add(ir::StoreLocalInstr::New(local, for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitStoreFieldExpr(expr::StoreFieldExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue() && expr->HasInstance());
  const auto field = expr->GetField();
  ASSERT(field);

  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(for_value)) {
    LOG(FATAL) << "failed to visit SetExpr value: " << expr->GetValue()->ToString();
    return false;
  }
  ASSERT(for_value.HasValue());
  Append(for_value);

  ValueVisitor for_instance(GetOwner());
  if (!expr->GetInstance()->Accept(for_instance)) {
    LOG(FATAL) << "failed to visit: " << expr->GetInstance()->ToString();
    return false;
  }
  ASSERT(for_instance.HasValue());
  Append(for_instance);

  Add(ir::StoreFieldInstr::New(field, for_instance.GetValue(), for_value.GetValue()));
  return true;
}

template <HasBodyExpr T>
auto EffectVisitor::Visit(T& target) -> bool {
  const auto scope = GetOwner()->PushScope();
  ASSERT(scope);
  if (target.HasScope())
    scope->AddAll(target.GetScope());
  ValueVisitor for_value(GetOwner());
  if (!for_value(target.GetBody())) {
    LOG(ERROR) << "failed to visit script body: " << target.GetBody()->ToString();
    return false;
  }
  if (for_value.IsOpen())
    for_value.AddImplicitReturn();
  ASSERT(for_value.IsClosed());
  Append(for_value);
  GetOwner()->PopScope();
  return true;
}

template auto EffectVisitor::Visit(Lambda& rhs) -> bool;
template auto EffectVisitor::Visit(Script& rhs) -> bool;

auto EffectVisitor::VisitSeqExpr(expr::SeqExpr* expr) -> bool {
  ASSERT(expr);
  auto index = 0;
  while (IsOpen() && (index < expr->GetNumberOfChildren())) {
    const auto child = expr->GetChildAt(index++);
    EffectVisitor for_value(GetOwner());
    LOG_IF(FATAL, !child->Accept(for_value)) << "failed to visit: " << child->ToString();
    Append(for_value);
    if (!IsOpen())
      break;
  }
  return true;
}

auto ValueVisitor::VisitDoExpr(expr::DoExpr* expr) -> bool {
  ASSERT(expr);
  ValueVisitor for_body(GetOwner());
  if (!for_body(expr->GetBody())) {
    LOG(ERROR) << "failed to visit do-expr body: " << expr->GetBody()->ToString();
    return false;
  }
  Append(for_body);
  if (for_body.HasValue()) {
    ReturnValue(for_body.GetValue());
  } else {
    ReturnNull();
  }
  return true;
}

auto ValueVisitor::VisitSeqExpr(expr::SeqExpr* seq) -> bool {
  ASSERT(seq);
  auto index = 0;
  ir::Definition* return_value = nullptr;
  const auto& body = seq->GetBody();
  while (IsOpen() && (index < body.size())) {
    const auto expr = body[index++];
    ASSERT(expr);
    ValueVisitor for_value(GetOwner());
    if (!for_value(expr)) {
      LOG(ERROR) << "failed to visit seq-expr child #" << index << ": " << expr->ToString();
      return false;
    }
    Append(for_value);
    return_value = for_value.GetValue();
  }
  if (return_value)
    ReturnValue(return_value);
  return true;
}

template <HasBodyExpr Target>
auto FlowGraphBuilder::Build(Target& target, LocalScope* scope) -> FlowGraph* {
  ASSERT(scope);
  FlowGraphBuilder builder(scope);
  const auto target_entry = ir::TargetEntryInstr::New(builder.GetNextBlockId());
  ASSERT(target_entry);
  builder.SetCurrentBlock(target_entry);
  EffectVisitor for_effect(&builder);
  if (!for_effect(target)) {
    LOG(ERROR) << "failed to visit: " << target;
    return nullptr;
  }
  AppendFragment(target_entry, for_effect);
  const auto graph_entry = ir::GraphEntryInstr::New(&target, builder.GetNextBlockId(), target_entry);
  ASSERT(graph_entry);
  graph_entry->Append(target_entry);
  graph_entry->AddDominated(target_entry);
  return new FlowGraph(&target, graph_entry);
}

template auto FlowGraphBuilder::Build(Lambda& target, LocalScope* scope) -> FlowGraph*;
template auto FlowGraphBuilder::Build(Script& target, LocalScope* scope) -> FlowGraph*;
}  // namespace gel
