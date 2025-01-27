#ifndef GEL_EXPRESSION_H
#define GEL_EXPRESSION_H

#include <ostream>
#include <string>
#include <utility>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/object.h"
#include "gel/symbol.h"
#include "gel/variable.h"

#define FOR_EACH_EXPRESSION_NODE(V) \
  V(LiteralExpr)                    \
  V(UnaryExpr)                      \
  V(BinaryOpExpr)                   \
  V(BeginExpr)                      \
  V(WhileExpr)                      \
  V(CondExpr)                       \
  V(ClauseExpr)                     \
  V(WhenExpr)                       \
  V(CaseExpr)                       \
  V(LocalDef)                       \
  V(ImportExpr)                     \
  V(CallProcExpr)                   \
  V(LoadInstanceMethodExpr)         \
  V(SetFieldExpr)                   \
  V(SetLocalExpr)                   \
  V(Binding)                        \
  V(LetExpr)                        \
  V(RxOpExpr)                       \
  V(LetRxExpr)                      \
  V(ListExpr)                       \
  V(ThrowExpr)                      \
  V(QuotedExpr)                     \
  V(InstanceOfExpr)                 \
  V(CastExpr)                       \
  V(NewExpr)                        \
  V(NewMapExpr)                     \
  V(LoadFieldExpr)

namespace gel {
class Parser;
class MacroEffectVisitor;

namespace expr {
namespace proto {
class HasSymbol {
  DEFINE_NON_COPYABLE_TYPE(HasSymbol);

 private:
  Symbol* symbol_;

 protected:
  explicit HasSymbol(Symbol* symbol = nullptr) :
    symbol_(symbol) {}

  void SetSymbol(Symbol* rhs) {
    ASSERT(rhs);
    symbol_ = rhs;
  }

 public:
  virtual ~HasSymbol() = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }
};
}  // namespace proto

class Expression;
class Definition;
#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_EXPRESSION_NODE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class ExpressionVisitor {
  DEFINE_NON_COPYABLE_TYPE(ExpressionVisitor);
#define FORWARD_DECLARE(Name) friend class Name;
  FOR_EACH_EXPRESSION_NODE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

 protected:
  ExpressionVisitor() = default;

#define DEFINE_VISIT(Name) virtual auto Visit##Name(Name* expr)->bool = 0;
  FOR_EACH_EXPRESSION_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
 public:
  virtual ~ExpressionVisitor() = default;
};

class Expression : public Object {  // TODO: should Expression inherit from Object?
  friend class Class;
  friend class Object;
  DEFINE_NON_COPYABLE_TYPE(Expression);

 protected:
  Expression() = default;

  virtual void SetChildAt(const uint64_t idx, Expression* expr) {
    // do nothing
  }

  virtual void RemoveChildAt(const uint64_t idx) {
    // do nothing
  }

 public:
  ~Expression() override = default;
  virtual auto GetName() const -> const char* = 0;
  virtual auto Accept(ExpressionVisitor* vis) -> bool = 0;

  virtual auto GetNumberOfChildren() const -> uint64_t {
    return 0;
  }

  virtual auto GetChildAt(const uint64_t idx) const -> Expression* {
    return nullptr;
  }

  inline auto HasChildAt(const uint64_t idx) const -> bool {
    return GetChildAt(idx) != nullptr;
  }

  inline auto HasChildren() const -> bool {
    return GetNumberOfChildren() >= 1;
  }

  virtual auto IsConstantExpr() const -> bool {
    return false;
  }

  virtual auto EvalToConstant(LocalScope* scope) const -> Object* {
    return nullptr;
  }

  virtual auto AsDefinition() -> Definition* {
    return nullptr;
  }

  inline auto IsDefinition() -> bool {
    return AsDefinition() != nullptr;
  }

  virtual auto VisitAllDefinitions(ExpressionVisitor* vis) -> bool {
    return true;
  }

  virtual auto VisitChildren(ExpressionVisitor* vis) -> bool {
    return false;
  }

  auto AsExpression() -> Expression* override {
    return this;
  }

  auto GetType() const -> Class* override {
    return GetClass();
  }

  auto HashCode() const -> uword override {
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return false;
  }

  auto Equals(Object* rhs) const -> bool override {
    ASSERT(rhs);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return false;
  }

#define DEFINE_TYPE_CHECK(Name)    \
  virtual auto As##Name()->Name* { \
    return nullptr;                \
  }                                \
  auto Is##Name()->bool {          \
    return As##Name() != nullptr;  \
  }
  FOR_EACH_EXPRESSION_NODE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
 private:
  static Class* kClass;

 public:
  static void Init();

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};

template <const int NumInputs>
class TemplateExpression : public Expression {
 private:
  std::array<Expression*, NumInputs> children_{};

 protected:
  TemplateExpression() = default;

  void SetChildAt(const uint64_t idx, Expression* value) override {
    ASSERT(idx >= 0 && idx <= NumInputs);
    ASSERT(value);
    children_.at(idx) = value;
  }

 public:
  auto GetNumberOfChildren() const -> uint64_t override {
    return NumInputs;
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= NumInputs);
    return children_.at(idx);
  }

  inline auto HasChildAt(const uint64_t idx) const -> bool {
    return GetChildAt(idx) != nullptr;
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override {
    ASSERT(vis);
    for (const auto& child : children_) {
      if (!child->Accept(vis))
        return false;
    }
    return true;
  }
};

using ExpressionList = std::vector<Expression*>;

static inline auto operator<<(std::ostream& stream, const ExpressionList& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& expr : rhs) {
    stream << expr->ToString();
    if (--remaining >= 1)
      stream << ", ";
  }
  stream << "]";
  return stream;
}

#define DECLARE_EXPRESSION(Name)                        \
  friend class ExpressionVisitor;                       \
  DEFINE_NON_COPYABLE_TYPE(Name);                       \
                                                        \
 public:                                                \
  static auto operator new(const size_t sz)->void*;     \
  static inline void operator delete(void* ptr) {       \
    ASSERT(ptr);                                        \
  }                                                     \
                                                        \
 public:                                                \
  auto Accept(ExpressionVisitor* vis) -> bool override; \
  auto ToString() const -> std::string override;        \
  auto GetName() const -> const char* override {        \
    return #Name;                                       \
  }                                                     \
  auto As##Name()->Name* override {                     \
    return this;                                        \
  }

class LiteralExpr : public Expression {
 private:
  Object* value_;

 protected:
  explicit LiteralExpr(Object* value) :
    Expression(),
    value_(value) {}

 public:
  ~LiteralExpr() override = default;

  auto GetValue() const -> Object* {
    return value_;
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  auto IsConstantExpr() const -> bool override {
    return true;
  }

  auto EvalToConstant(LocalScope* scope) const -> Object* override {
    return value_;
  }

  DECLARE_EXPRESSION(LiteralExpr);

 public:
  static inline auto New(Object* value) -> LiteralExpr* {
    ASSERT(value);
    return new LiteralExpr(value);
  }
};

#define FOR_EACH_BINARY_OP(V) \
  V(Add)                      \
  V(Subtract)                 \
  V(Multiply)                 \
  V(Divide)                   \
  V(Modulus)                  \
  V(Equals)                   \
  V(BinaryAnd)                \
  V(BinaryOr)                 \
  V(GreaterThan)              \
  V(GreaterThanEqual)         \
  V(LessThan)                 \
  V(LessThanEqual)            \
  V(Cons)                     \
  V(InstanceOf)

template <class Op, const uword NumInputs>
class TemplateOpExpression : public TemplateExpression<NumInputs> {
  DEFINE_NON_COPYABLE_TYPE(TemplateOpExpression);

 private:
  Op op_;

 protected:
  explicit TemplateOpExpression(const Op op) :
    TemplateExpression<NumInputs>(),
    op_(op) {}

  void SetOp(const Op op) {
    op_ = op;
  }

 public:
  ~TemplateOpExpression() override = default;

  auto GetOp() const -> Op {
    return op_;
  }
};

enum BinaryOp : uint64_t {
#define DEFINE_BINARY_OP(Name) k##Name,
  FOR_EACH_BINARY_OP(DEFINE_BINARY_OP)
#undef DEFINE_BINARY_OP
};

static inline constexpr auto BinaryOpToString(const BinaryOp& rhs) -> const char* {
  switch (rhs) {
#define DEFINE_TO_STRING(Name) \
  case BinaryOp::k##Name:      \
    return #Name;
    FOR_EACH_BINARY_OP(DEFINE_TO_STRING)
#undef DEFINE_TO_STRING
    default:
      return "Invalid Binary Op";
  }
}

static inline auto operator<<(std::ostream& stream, const BinaryOp& rhs) -> std::ostream& {
  switch (rhs) {
#define DEFINE_TO_STRING(Name) \
  case BinaryOp::k##Name:      \
    return stream << #Name;
    FOR_EACH_BINARY_OP(DEFINE_TO_STRING)
#undef DEFINE_TO_STRING
    default:
      return stream << "Unknown BinaryOp: " << static_cast<int64_t>(rhs);
  }
}

class BinaryOpExpr : public TemplateOpExpression<BinaryOp, 2> {
  static constexpr const auto kLeftInput = 0;
  static constexpr const auto kRightInput = 1;

 protected:
  explicit BinaryOpExpr(const BinaryOp op, Expression* left, Expression* right) :
    TemplateOpExpression<BinaryOp, 2>(op) {
    SetLeft(left);
    SetRight(right);
  }

 public:
  ~BinaryOpExpr() override = default;

  auto GetLeft() const -> Expression* {
    return GetChildAt(kLeftInput);
  }

  inline auto HasLeft() const -> bool {
    return GetLeft() != nullptr;
  }

  inline void SetLeft(Expression* value) {
    ASSERT(value);
    SetChildAt(kLeftInput, value);
  }

  auto GetRight() const -> Expression* {
    return GetChildAt(kRightInput);
  }

  inline auto HasRight() const -> bool {
    return GetRight() != nullptr;
  }

  inline void SetRight(Expression* value) {
    ASSERT(value);
    SetChildAt(kRightInput, value);
  }

#define DEFINE_OP_CHECK(Name)              \
  inline auto Is##Name##Op() const->bool { \
    return GetOp() == BinaryOp::k##Name;   \
  }
  FOR_EACH_BINARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK

  auto IsConstantExpr() const -> bool override;
  auto EvalToConstant(LocalScope* scope) const -> Object* override;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(BinaryOpExpr);

 public:
  static inline auto New(const BinaryOp op, Expression* left, Expression* right) -> BinaryOpExpr* {
    ASSERT(left);
    ASSERT(right);
    return new BinaryOpExpr(op, left, right);
  }

#define DEFINE_NEW_OP(Name)                                                       \
  static inline auto New##Name(Expression* lhs, Expression* rhs)->BinaryOpExpr* { \
    ASSERT(lhs);                                                                  \
    ASSERT(rhs);                                                                  \
    return New(BinaryOp::k##Name, lhs, rhs);                                      \
  }
  FOR_EACH_BINARY_OP(DEFINE_NEW_OP)
#undef DEFINE_NEW_OP
};

#define FOR_EACH_UNARY_OP(V) \
  V(Not)                     \
  V(Car)                     \
  V(Cdr)                     \
  V(Nonnull)                 \
  V(Null)

enum UnaryOp : uint64_t {
#define DEFINE_UNARY_OP(Name) k##Name,
  FOR_EACH_UNARY_OP(DEFINE_UNARY_OP)
#undef DEFINE_UNARY_OP
};

static inline auto operator<<(std::ostream& stream, const UnaryOp& rhs) -> std::ostream& {
  switch (rhs) {
#define DEFINE_TO_STRING(Name) \
  case UnaryOp::k##Name:       \
    return stream << #Name;
    FOR_EACH_UNARY_OP(DEFINE_TO_STRING)
#undef DEFINE_TO_STRING
    default:
      return stream << "Unknown UnaryOp: " << static_cast<uint64_t>(rhs);
  }
}

class UnaryExpr : public TemplateOpExpression<UnaryOp, 1> {
 protected:
  UnaryExpr(const UnaryOp op, Expression* value) :
    TemplateOpExpression<UnaryOp, 1>(op) {
    SetChildAt(0, value);
  }

 public:
  ~UnaryExpr() override = default;

  inline auto GetValue() const -> Expression* {
    return GetChildAt(0);
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  inline void SetValue(Expression* expr) {
    ASSERT(expr);
    SetChildAt(0, expr);
  }

#define DEFINE_OP_CHECK(Name)              \
  inline auto Is##Name##Op() const->bool { \
    return GetOp() == UnaryOp::k##Name;    \
  }
  FOR_EACH_UNARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK

  DECLARE_EXPRESSION(UnaryExpr);

 public:
  static inline auto New(const UnaryOp op, Expression* value) -> UnaryExpr* {
    ASSERT(value);
    return new UnaryExpr(op, value);
  }

#define DEFINE_NEW_OP(Name)                                     \
  static inline auto New##Name(Expression* value)->UnaryExpr* { \
    ASSERT(value);                                              \
    return New(UnaryOp::k##Name, value);                        \
  }
  FOR_EACH_UNARY_OP(DEFINE_NEW_OP)
#undef DEFINE_NEW_OP
};

class ThrowExpr : public TemplateExpression<1> {
 protected:
  explicit ThrowExpr(Expression* value) {
    SetValue(value);
  }

  inline void SetValue(Expression* expr) {
    ASSERT(expr);
    SetChildAt(0, expr);
  }

 public:
  ~ThrowExpr() override = default;

  inline auto GetValue() const -> Expression* {
    return GetChildAt(0);
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  DECLARE_EXPRESSION(ThrowExpr);

 public:
  static inline auto New(Expression* value) -> ThrowExpr* {
    ASSERT(value);
    return new ThrowExpr(value);
  }
};

class QuotedExpr : public Expression {
 private:
  Symbol* value_;

 protected:
  explicit QuotedExpr(Symbol* value) :
    Expression(),
    value_(value) {
    ASSERT(value_);
  }

 public:
  ~QuotedExpr() override = default;

  auto Get() const -> Symbol* {
    return value_;
  }

  DECLARE_EXPRESSION(QuotedExpr);

 public:
  static inline auto New(Symbol* symbol) -> QuotedExpr* {
    ASSERT(symbol);
    return new QuotedExpr(symbol);
  }

  static inline auto New(const std::string& value) -> QuotedExpr* {
    ASSERT(!value.empty());
    return New(Symbol::New(value));
  }
};

class SequenceExpr : public Expression {
  friend class gel::Parser;
  DEFINE_NON_COPYABLE_TYPE(SequenceExpr);

 private:
  ExpressionList children_{};

 protected:
  SequenceExpr(const ExpressionList& children) {
    children_.insert(std::end(children_), std::begin(children), std::end(children));
  }

 public:
  ~SequenceExpr() override = default;

  auto GetBody() const -> const ExpressionList& {
    return children_;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return children_.size();
  }

  virtual auto IsEmpty() const -> bool {
    return GetNumberOfChildren() == 0;
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return children_[idx];
  }

  void SetChildAt(const uint64_t idx, Expression* value) override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    ASSERT(value);
    children_[idx] = value;
  }

  inline void Append(Expression* expr) {
    ASSERT(expr);
    children_.push_back(expr);
  }

  void RemoveChildAt(const uint64_t idx) override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    children_.erase(children_.begin() + static_cast<word>(idx));
  }

  auto GetLastExpr() const -> Expression* {
    return IsEmpty() ? nullptr : children_.back();
  }

  auto IsConstantExpr() const -> bool override;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  auto VisitAllDefinitions(ExpressionVisitor* vis) -> bool override;
};

class BeginExpr : public SequenceExpr {
 protected:
  explicit BeginExpr(const ExpressionList& expressions) :
    SequenceExpr(expressions) {}

 public:
  ~BeginExpr() override = default;

  DECLARE_EXPRESSION(BeginExpr);

 public:
  static inline auto New(const ExpressionList& expressions = {}) -> BeginExpr* {
    return new BeginExpr(expressions);
  }
};

class CallProcExpr : public Expression {
  friend class gel::MacroEffectVisitor;

 private:
  Expression* target_ = nullptr;
  ExpressionList args_{};

 protected:
  explicit CallProcExpr(Expression* target, const ExpressionList& args) :  // NOLINT(modernize-pass-by-value)
    Expression(),
    args_(args) {
    SetTarget(target);
  }

  inline void SetTarget(Expression* target) {
    ASSERT(target);
    target_ = target;
  }

  void SetArgAt(const uint64_t idx, Expression* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfArgs());
    ASSERT(expr);
    args_[idx] = expr;
  }

 public:
  ~CallProcExpr() override = default;

  auto GetTarget() const -> Expression* {
    return target_;
  }

  inline auto HasTarget() const -> bool {
    return GetTarget() != nullptr;
  }

  inline auto GetNumberOfArgs() const -> uint64_t {
    return args_.size();
  }

  auto GetArgs() const -> const ExpressionList& {
    return args_;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return GetNumberOfArgs() + 1;
  }

  auto GetArgAt(const uint64_t idx) const -> Expression* {
    return GetChildAt(idx + 1);
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return idx == 0 ? GetTarget() : args_[idx - 1];
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override {
    ASSERT(vis);
    if (!GetTarget()->Accept(vis))
      return false;
    for (const auto& arg : args_) {
      if (!arg->Accept(vis))
        return false;
    }
    return true;
  }

  auto IsMacroCall(LocalScope* scope) const -> bool;
  DECLARE_EXPRESSION(CallProcExpr);

 public:
  static inline auto New(Expression* target, const ExpressionList& args = {}) -> CallProcExpr* {
    return new CallProcExpr(target, args);
  }
};

class ClauseExpr : public Expression {  // TODO: should this be a WhenExpr?
 private:
  Expression* key_;
  ExpressionList actions_;

  ClauseExpr(Expression* key, const ExpressionList& actions) :  // NOLINT(modernize-pass-by-value)
    Expression(),
    key_(key),
    actions_(actions) {
    ASSERT(key_);
    ASSERT(!actions_.empty());
  }

 public:
  ~ClauseExpr() override = default;

  auto GetKey() const -> Expression* {
    return key_;
  }

  auto GetActions() const -> const ExpressionList& {
    return actions_;
  }

  auto GetNumberOfActions() const -> uint64_t {
    return actions_.size();
  }

  auto GetActionAt(const uint64_t idx) const -> Expression* {
    ASSERT(idx >= 0 && idx <= GetNumberOfActions());
    return actions_[idx];
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return 1 + GetNumberOfActions();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return idx == 0 ? GetKey() : GetActionAt(idx - 1);
  }

  auto VisitAllActions(ExpressionVisitor* vis) -> bool;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(ClauseExpr);

 public:
  static inline auto New(Expression* key, const ExpressionList& actions = {}) -> ClauseExpr* {
    ASSERT(key);
    ASSERT(!actions.empty());
    return new ClauseExpr(key, actions);
  }

  static inline auto New(Expression* key, Expression* action) -> ClauseExpr* {
    ASSERT(key);
    ASSERT(action);
    return New(key, ExpressionList{action});
  }
};

using ClauseList = std::vector<ClauseExpr*>;

static inline auto operator<<(std::ostream& stream, const ClauseList& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& clause : rhs) {
    stream << clause->ToString();
    if (--remaining >= 1)
      stream << ", ";
  }
  stream << "]";
  return stream;
}

class CondExpr : public Expression {
 private:
  ClauseList clauses_;
  Expression* alt_ = nullptr;

 protected:
  CondExpr(const ClauseList& clauses, Expression* alt) :
    Expression(),
    clauses_(clauses),
    alt_(alt) {
    ASSERT(!clauses_.empty());
  }

 public:
  ~CondExpr() override = default;

  auto GetClauses() const -> const ClauseList& {
    return clauses_;
  }

  auto GetNumberOfClauses() const -> uint64_t {
    return clauses_.size();
  }

  auto GetClauseAt(const uint64_t idx) const -> ClauseExpr* {
    ASSERT(idx >= 0 && idx <= GetNumberOfClauses());
    return clauses_[idx];
  }

  void SetClauseAt(const uint64_t idx, ClauseExpr* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfClauses());
    clauses_[idx] = expr;
  }

  auto GetAlternate() const -> Expression* {
    return alt_;
  }

  inline auto HasAlternate() const -> bool {
    return GetAlternate() != nullptr;
  }

  inline void SetAlt(Expression* expr) {
    ASSERT(expr);
    alt_ = expr;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return (HasAlternate() ? 1 : 0) + GetNumberOfClauses();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx < GetNumberOfChildren());
    return (idx >= 0 && idx <= GetNumberOfClauses()) ? clauses_[idx] : GetAlternate();
  }

  auto VisitAllClauses(ExpressionVisitor* vis) -> bool;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(CondExpr);

 public:
  static inline auto New(const ClauseList& clauses = {}, Expression* alt = nullptr) -> CondExpr* {
    ASSERT(!clauses.empty());
    return new CondExpr(clauses, alt);
  }

  static inline auto New(Expression* test, Expression* conseq, Expression* alt = nullptr) -> CondExpr* {
    ASSERT(test);
    ASSERT(conseq);
    return New(
        ClauseList{
            ClauseExpr::New(test, conseq),
        },
        alt);
  }
};

class WhenExpr : public Expression {
  friend class gel::MacroEffectVisitor;

 private:
  Expression* test_;
  ExpressionList actions_;

 protected:
  WhenExpr(Expression* test, const ExpressionList& actions) :
    Expression(),
    test_(test),
    actions_(actions) {
    ASSERT(test_);
    ASSERT(!actions_.empty());
  }

  void SetTest(Expression* test) {
    ASSERT(test);
    test_ = test;
  }

  void SetActions(const ExpressionList& actions) {
    ASSERT(!actions.empty());
    actions_ = actions;
  }

  void SetActionAt(const uint64_t idx, Expression* expr) {
    ASSERT(idx >= 0 && idx <= GetNumberOfActions());
    actions_[idx] = expr;
  }

 public:
  ~WhenExpr() override = default;

  auto GetTest() const -> Expression* {
    return test_;
  }

  auto GetActions() const -> const ExpressionList& {
    return actions_;
  }

  auto GetNumberOfActions() const -> uint64_t {
    return actions_.size();
  }

  auto GetActionAt(const uint64_t idx) const -> Expression* {
    ASSERT(idx >= 0 && idx <= GetNumberOfActions());
    return actions_[idx];
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return 1 + GetNumberOfActions();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return idx == 0 ? GetTest() : GetActionAt(idx - 1);
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(WhenExpr);

 public:
  static inline auto New(Expression* test, const ExpressionList& actions = {}) -> WhenExpr* {
    ASSERT(test);
    ASSERT(!actions.empty());
    return new WhenExpr(test, actions);
  }

  static inline auto New(Expression* test, Expression* action) -> WhenExpr* {
    ASSERT(test);
    ASSERT(action);
    return New(test, ExpressionList{action});
  }
};

class CaseExpr : public Expression {
 private:
  Expression* key_;
  ClauseList clauses_;

  explicit CaseExpr(Expression* key, const ClauseList& clauses) :  // NOLINT(modernize-pass-by-value)
    Expression(),
    key_(key),
    clauses_(clauses) {}

 public:
  ~CaseExpr() override = default;

  inline void SetKey(Expression* expr) {
    ASSERT(expr);
    key_ = expr;
  }

  auto GetKey() const -> Expression* {
    return key_;
  }

  auto GetClauses() const -> const ClauseList& {
    return clauses_;
  }

  auto GetNumberOfClauses() const -> uint64_t {
    return clauses_.size();
  }

  auto GetClauseAt(const uint64_t idx) const -> ClauseExpr* {
    ASSERT(idx >= 0 && idx <= GetNumberOfClauses());
    return clauses_[idx];
  }

  auto VisitAllClauses(ExpressionVisitor* vis) -> bool;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(CaseExpr);

 public:
  static inline auto New(Expression* key, const ClauseList& clauses = {}) -> CaseExpr* {
    return new CaseExpr(key, clauses);
  }
};

class WhileExpr : public SequenceExpr {
 private:
  Expression* test_;

 protected:
  explicit WhileExpr(Expression* test, const ExpressionList& body) :
    SequenceExpr(body),
    test_(test) {}

 public:
  ~WhileExpr() override = default;

  auto GetTest() const -> Expression* {
    return test_;
  }

  DECLARE_EXPRESSION(WhileExpr);

 public:
  static inline auto New(Expression* test, const ExpressionList& body = {}) -> WhileExpr* {
    return new WhileExpr(test, body);
  }
};

class SetLocalExpr : public Expression {
  friend class gel::MacroEffectVisitor;

 private:
  LocalVariable* local_;
  Expression* value_;

 protected:
  SetLocalExpr(LocalVariable* local, Expression* value) :
    Expression(),
    local_(local),
    value_(value) {
    ASSERT(local_);
  }

  void SetValue(Expression* rhs) {
    ASSERT(rhs);
    value_ = rhs;
  }

 public:
  ~SetLocalExpr() override = default;

  auto GetLocal() const -> LocalVariable* {
    return local_;
  }

  auto GetValue() const -> Expression* {
    return value_;
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override {
    ASSERT(vis);
    if (!HasValue())
      return false;
    return GetValue()->Accept(vis);
  }

  DECLARE_EXPRESSION(SetLocalExpr);

 public:
  static inline auto New(LocalVariable* local, Expression* value) -> SetLocalExpr* {
    ASSERT(local);
    return new SetLocalExpr(local, value);
  }
};

class SetFieldExpr : public Expression {
  friend class gel::MacroEffectVisitor;

 private:
  Field* field_;
  Expression* instance_;
  Expression* value_;

 protected:
  SetFieldExpr(Field* field, Expression* instance, Expression* value) :
    Expression(),
    field_(field),
    instance_(instance),
    value_(value) {
    ASSERT(field_);
    ASSERT(instance_);
    ASSERT(value_);
  }

  void SetValue(Expression* rhs) {
    ASSERT(rhs);
    value_ = rhs;
  }

 public:
  ~SetFieldExpr() override = default;

  auto GetField() const -> Field* {
    return field_;
  }

  auto GetInstance() const -> Expression* {
    return instance_;
  }

  inline auto HasInstance() const -> bool {
    return GetInstance() != nullptr;
  }

  auto GetValue() const -> Expression* {
    return value_;
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override {
    ASSERT(vis);
    if (HasInstance() && !GetInstance()->Accept(vis))
      return false;
    if (!HasValue() && !GetValue()->Accept(vis))
      return false;
    return true;
  }

  DECLARE_EXPRESSION(SetFieldExpr);

 public:
  static inline auto New(Field* field, Expression* instance, Expression* value) -> SetFieldExpr* {
    ASSERT(field);
    return new SetFieldExpr(field, instance, value);
  }
};

class Binding : public Expression {
 private:
  LocalVariable* local_;
  Expression* value_;

 protected:
  Binding(LocalVariable* local, Expression* value) :
    local_(local),
    value_(value) {}

 public:
  ~Binding() override = default;

  auto GetLocal() const -> LocalVariable* {
    return local_;
  }

  auto GetValue() const -> Expression* {
    return value_;
  }

  DECLARE_EXPRESSION(Binding);

 public:
  static inline auto New(LocalVariable* local, Expression* value) -> Binding* {
    return new Binding(local, value);
  }
};

class LoadFieldExpr : public TemplateExpression<1> {
 private:
  Field* field_;

  LoadFieldExpr(Expression* instance, Field* field) :
    TemplateExpression(),
    field_(field) {
    SetInstance(instance);
    ASSERT(field_);
    ASSERT(HasInstance());
  }

  void SetInstance(Expression* expr) {
    ASSERT(expr);
    SetChildAt(0, expr);
  }

 public:
  ~LoadFieldExpr() override = default;

  auto GetField() const -> Field* {
    return field_;
  }

  inline auto GetInstance() const -> Expression* {
    return GetChildAt(0);
  }

  inline auto HasInstance() const -> bool {
    return GetInstance() != nullptr;
  }

  DECLARE_EXPRESSION(LoadFieldExpr);

 public:
  static inline auto New(Expression* instance, Field* field) -> LoadFieldExpr* {
    ASSERT(instance);
    ASSERT(field);
    return new LoadFieldExpr(instance, field);
  }
};

class ImportExpr : public Expression {
 private:
  Module* module_;

  explicit ImportExpr(Module* module) :
    Expression(),
    module_(module) {
    ASSERT(module_);
  }

 public:
  ~ImportExpr() override = default;

  auto GetModule() const -> Module* {
    return module_;
  }

  DECLARE_EXPRESSION(ImportExpr);

 public:
  static inline auto New(Module* module) -> ImportExpr* {
    ASSERT(module);
    return new ImportExpr(module);
  }
};

using BindingList = std::vector<Binding*>;

static inline auto operator<<(std::ostream& stream, const BindingList& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& value : rhs) {
    ASSERT(value);
    stream << value;
    if (--remaining > 0)
      stream << ", ";
  }
  stream << "]";
  return stream;
}

class TemplateLetExpr : public SequenceExpr {
  DEFINE_NON_COPYABLE_TYPE(TemplateLetExpr);

 private:
  LocalScope* scope_;

 protected:
  explicit TemplateLetExpr(LocalScope* scope, const ExpressionList& body) :
    SequenceExpr(body),
    scope_(scope) {
    ASSERT(scope_);
  }

 public:
  ~TemplateLetExpr() override = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }
};

class RxOpExpr : public SequenceExpr, public proto::HasSymbol {
 protected:
  RxOpExpr(Symbol* symbol, const ExpressionList& body) :
    SequenceExpr(body),
    proto::HasSymbol(symbol) {}

 public:
  ~RxOpExpr() override = default;

  auto IsComplete() const -> bool;
  auto IsSubscribe() const -> bool;
  DECLARE_EXPRESSION(RxOpExpr);

 public:
  static inline auto New(Symbol* symbol, const ExpressionList& body = {}) -> RxOpExpr* {
    ASSERT(symbol);
    return new RxOpExpr(symbol, body);
  }
};

using RxOpList = std::vector<RxOpExpr*>;

class LetRxExpr : public TemplateLetExpr {
 private:
  Expression* source_;

 protected:
  LetRxExpr(LocalScope* scope, Expression* observable, const RxOpList& operators) :
    TemplateLetExpr(scope, (const ExpressionList&)operators),  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    source_(observable) {}

 public:
  ~LetRxExpr() override = default;

  auto GetSource() const -> Expression* {
    return source_;
  }

  auto GetNumberOfOperators() const -> uint64_t {
    return GetNumberOfChildren();
  }

  auto GetOperatorAt(const uint64_t idx) const -> RxOpExpr* {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return (RxOpExpr*)GetChildAt(idx);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto GetLastOp() const -> RxOpExpr* {
    if (IsEmpty())
      return nullptr;
    return (RxOpExpr*)GetLastExpr();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto IsConstantExpr() const -> bool override {
    return false;
  }

  auto HasSubscribe() const -> bool;
  auto HasComplete() const -> bool;
  DECLARE_EXPRESSION(LetRxExpr);

 public:
  static inline auto New(LocalScope* scope, Expression* source, const RxOpList& body = {}) -> LetRxExpr* {
    return new LetRxExpr(scope, source, body);
  }
};

class LetExpr : public TemplateLetExpr {
 private:
  BindingList bindings_;

 protected:
  LetExpr(LocalScope* scope, const BindingList& bindings, const ExpressionList& body) :
    TemplateLetExpr(scope, body),
    bindings_(bindings) {}

 public:
  ~LetExpr() override = default;

  auto GetBindings() const -> const BindingList& {
    return bindings_;
  }

  auto GetNumberOfBindings() const -> uword {
    return bindings_.size();
  }

  auto GetBindingAt(const uint64_t idx) const -> Binding* {
    ASSERT(idx >= 0 && idx <= GetNumberOfBindings());
    return bindings_[idx];
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    if (idx >= GetNumberOfBindings())
      return SequenceExpr::GetChildAt(idx - GetNumberOfBindings());
    ASSERT(idx >= 0 && idx <= GetNumberOfBindings());
    return GetBindingAt(idx);
  }

  inline auto HasBindings() const -> bool {
    return GetNumberOfBindings() > 0;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return SequenceExpr::GetNumberOfChildren() + GetNumberOfBindings();
  }

  auto IsConstantExpr() const -> bool override {
    return false;
  }

  auto VisitAllBindings(ExpressionVisitor* vis) -> bool;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(LetExpr);

 public:
  static inline auto New(LocalScope* scope, const BindingList& bindings = {}, const ExpressionList& body = {}) -> LetExpr* {
    ASSERT(scope);
    return new LetExpr(scope, bindings, body);
  }
};

class ListExpr : public SequenceExpr {
 private:
 protected:
  explicit ListExpr(const ExpressionList& values) :
    SequenceExpr(values) {}

 public:
  ~ListExpr() override = default;

  auto IsConstantExpr() const -> bool override;
  auto EvalToConstant(LocalScope* scope) const -> Object* override;
  DECLARE_EXPRESSION(ListExpr);

 public:
  static inline auto New(const ExpressionList& values = {}) -> ListExpr* {
    return new ListExpr(values);
  }
};

class InstanceOfExpr : public TemplateExpression<1> {
  static constexpr const auto kValueIndex = 0;

 private:
  Class* target_;

 protected:
  InstanceOfExpr(Class* target, Expression* value) :
    TemplateExpression<1>(),
    target_(target) {
    ASSERT(target_);
    ASSERT(value);
    SetValue(value);
  }

  inline void SetValue(Expression* rhs) {
    ASSERT(rhs);
    SetChildAt(kValueIndex, rhs);
  }

 public:
  ~InstanceOfExpr() override = default;

  auto GetTarget() const -> Class* {
    return target_;
  }

  auto GetValue() const -> Expression* {
    return GetChildAt(kValueIndex);
  }

  auto IsConstantExpr() const -> bool override;
  auto EvalToConstant(LocalScope* scope) const -> Object* override;
  DECLARE_EXPRESSION(InstanceOfExpr);

 public:
  static inline auto New(Class* target, Expression* value) -> InstanceOfExpr* {
    ASSERT(target);
    ASSERT(value);
    return new InstanceOfExpr(target, value);
  }
};

class CastExpr : public TemplateExpression<1> {
  static constexpr const auto kValueIndex = 0;

 private:
  Class* target_;

 protected:
  CastExpr(Class* cls, Expression* value) :
    TemplateExpression(),
    target_(cls) {
    ASSERT(target_);
    ASSERT(value);
    SetValue(value);
  }

  inline void SetValue(Expression* expr) {
    ASSERT(expr && !HasChildAt(kValueIndex));
    SetChildAt(kValueIndex, expr);
  }

 public:
  ~CastExpr() override = default;

  auto GetTargetType() const -> Class* {
    return target_;
  }

  inline auto GetValue() const -> Expression* {
    return GetChildAt(kValueIndex);
  }

  DECLARE_EXPRESSION(CastExpr);

 public:
  static inline auto New(Class* cls, Expression* value) -> CastExpr* {
    ASSERT(cls);
    ASSERT(value);
    return new CastExpr(cls, value);
  }
};

class NewExpr : public Expression {
 private:
  Class* target_;
  ExpressionList args_;

 protected:
  NewExpr(Class* target, const ExpressionList& args) :
    target_(target),
    args_(args) {}

 public:
  ~NewExpr() override = default;

  auto GetTargetClass() const -> Class* {
    return target_;
  }

  auto GetArgs() const -> const ExpressionList& {
    return args_;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return args_.size();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return args_[idx];
  }

  auto VisitArgs(ExpressionVisitor* vis) -> bool;

  auto VisitChildren(ExpressionVisitor* vis) -> bool override {
    ASSERT(vis);
    return VisitArgs(vis);
  }

  auto EvalToConstant(LocalScope* scope) const -> Object* override;
  auto IsConstantExpr() const -> bool override;
  DECLARE_EXPRESSION(NewExpr);

 public:
  static inline auto New(Class* target, const ExpressionList& args) -> NewExpr* {
    return new NewExpr(target, args);
  }
};

class LoadInstanceMethodExpr : Expression {
 private:
  Class* class_;
  Symbol* name_;

  LoadInstanceMethodExpr(Class* cls, Symbol* name) :
    Expression(),
    class_(cls),
    name_(name) {
    ASSERT(class_);
    ASSERT(name_);
  }

 public:
  ~LoadInstanceMethodExpr() override = default;

  auto GetTargetClass() const -> Class* {
    return class_;
  }

  auto GetTargetName() const -> Symbol* {
    return name_;
  }

  auto IsConstantExpr() const -> bool override;
  auto EvalToConstant(LocalScope* scope) const -> Object* override;
  DECLARE_EXPRESSION(LoadInstanceMethodExpr);

 public:
  static inline auto New(Class* cls, Symbol* name) -> LoadInstanceMethodExpr* {
    ASSERT(cls);
    return new LoadInstanceMethodExpr(cls, name);
  }
};

class NewMapExpr : public Expression {
 public:
  using Entry = std::pair<Symbol*, Expression*>;
  using EntryList = std::vector<Entry>;

 private:
  EntryList data_;

  explicit NewMapExpr(const EntryList& data) :
    Expression(),
    data_(data) {}

  void RemoveChildAt(const uint64_t idx) override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    data_.erase(begin() + static_cast<EntryList::difference_type>(idx));
  }

  void SetChildAt(const uint64_t idx, Expression* value) override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    data_[idx].second = value;
  }

 public:
  ~NewMapExpr() override = default;

  auto data() const -> const EntryList& {
    return data_;
  }

  auto begin() const -> EntryList::const_iterator {
    return std::begin(data());
  }

  auto end() const -> EntryList::const_iterator {
    return std::end(data());
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return data().size();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return data()[idx].second;
  }

  auto IsEmpty() const -> bool {
    return data().empty();
  }

  auto IsConstantExpr() const -> bool override;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  auto EvalToConstant(LocalScope* scope) const -> Object* override;
  DECLARE_EXPRESSION(NewMapExpr);

 public:
  static inline auto New(const EntryList& data = {}) -> NewMapExpr* {
    return new NewMapExpr(data);
  }
};

// Definitions

class Definition : public Expression {
  DEFINE_NON_COPYABLE_TYPE(Definition);

 protected:
  Definition() = default;

 public:
  ~Definition() override = default;

  auto AsDefinition() -> Definition* override {
    return this;
  }
};

using DefinitionList = std::vector<Definition*>;

static inline auto operator<<(std::ostream& stream, const DefinitionList& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& defn : rhs) {
    stream << defn->ToString();
    if (--remaining >= 1)
      stream << ",";
  }
  stream << "]";
  return stream;
}

template <const uint64_t NumInputs>
class TemplateDefinition : public Definition {
  DEFINE_NON_COPYABLE_TYPE(TemplateDefinition<NumInputs>);

 private:
  std::array<Expression*, NumInputs> children_{};

 protected:
  TemplateDefinition() = default;

  void SetChildAt(const uint64_t idx, Expression* value) override {
    ASSERT(idx >= 0 && idx <= NumInputs);
    ASSERT(value);
    children_[idx] = value;  // NOLINT
  }

 public:
  ~TemplateDefinition() override = default;

  auto GetNumberOfChildren() const -> uint64_t override {
    return NumInputs;
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= NumInputs);
    return children_[idx];  // NOLINT
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override {
    ASSERT(vis);
    for (const auto& child : children_) {
      if (child && !child->Accept(vis))
        return false;
    }
    return true;
  }
};

class LocalDef : public TemplateDefinition<1> {
 private:
  LocalVariable* local_;

 protected:
  LocalDef(LocalVariable* local, Expression* value) :
    TemplateDefinition<1>(),
    local_(local) {
    SetChildAt(0, value);
  }

 public:
  ~LocalDef() override = default;

  auto GetLocal() const -> LocalVariable* {
    return local_;
  }

  auto GetValue() const -> Expression* {
    return GetChildAt(0);
  }

  auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  DECLARE_EXPRESSION(LocalDef);

 public:
  static inline auto New(LocalVariable* local, Expression* value) -> LocalDef* {
    ASSERT(local);
    ASSERT(value);
    return new LocalDef(local, value);
  }
};

static inline auto IsLiteralSymbol(Expression* rhs) -> bool {
  if (!rhs || !rhs->IsLiteralExpr())
    return false;
  const auto literal = rhs->AsLiteralExpr()->GetValue();
  return literal && literal->IsSymbol();
}

static inline auto IsLiteralSymbol(Expression* rhs, Symbol* value) -> bool {
  if (!rhs || !rhs->IsLiteralExpr())
    return false;
  const auto literal = rhs->AsLiteralExpr()->GetValue();
  return literal && literal->Equals(value);
}
}  // namespace expr

using expr::BinaryOp;
using expr::Expression;
using expr::ExpressionList;
using expr::ExpressionVisitor;
#define DEFINE_USE(Name) using expr::Name;
FOR_EACH_EXPRESSION_NODE(DEFINE_USE)
#undef DEFINE_USE
}  // namespace gel

#undef DECLARE_EXPRESSION
#endif  // GEL_EXPRESSION_H
