#ifndef SCM_EXPRESSION_H
#define SCM_EXPRESSION_H

#include <ostream>
#include <string>
#include <utility>

#include "scheme/common.h"
#include "scheme/lambda.h"
#include "scheme/type.h"
#include "scheme/variable.h"

#define FOR_EACH_EXPRESSION_NODE(V) \
  V(LiteralExpr)                    \
  V(UnaryExpr)                      \
  V(BinaryOpExpr)                   \
  V(BeginExpr)                      \
  V(CondExpr)                       \
  V(ConsExpr)                       \
  V(LambdaExpr)                     \
  V(LocalDef)                       \
  V(ModuleDef)                      \
  V(ImportDef)                      \
  V(MacroDef)                       \
  V(EvalExpr)                       \
  V(CallProcExpr)                   \
  V(SetExpr)                        \
  V(ThrowExpr)                      \
  V(QuotedExpr)

namespace scm {
class Parser;

namespace expr {
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

#define DEFINE_VISIT(Name) virtual auto Visit##Name(Name* expr) -> bool = 0;
  FOR_EACH_EXPRESSION_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
 public:
  virtual ~ExpressionVisitor() = default;
};

class Expression {
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
  virtual ~Expression() = default;
  virtual auto GetName() const -> const char* = 0;
  virtual auto ToString() const -> std::string = 0;
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

  virtual auto IsConstantExpr() const -> bool {
    return false;
  }

  virtual auto EvalToConstant() const -> Type* {
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

#define DEFINE_TYPE_CHECK(Name)      \
  virtual auto As##Name() -> Name* { \
    return nullptr;                  \
  }                                  \
  auto Is##Name() -> bool {          \
    return As##Name() != nullptr;    \
  }
  FOR_EACH_EXPRESSION_NODE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
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

#define DECLARE_EXPRESSION(Name)                        \
  friend class ExpressionVisitor;                       \
  DEFINE_NON_COPYABLE_TYPE(Name);                       \
                                                        \
 public:                                                \
  auto Accept(ExpressionVisitor* vis) -> bool override; \
  auto ToString() const -> std::string override;        \
  auto GetName() const -> const char* override {        \
    return #Name;                                       \
  }                                                     \
  auto As##Name() -> Name* override {                   \
    return this;                                        \
  }

class LiteralExpr : public Expression {
 private:
  Datum* value_;

 protected:
  explicit LiteralExpr(Datum* value) :
    Expression(),
    value_(value) {}

 public:
  ~LiteralExpr() override = default;

  auto GetValue() const -> Datum* {
    return value_;
  }

  auto IsConstantExpr() const -> bool override {
    return true;
  }

  auto EvalToConstant() const -> Type* override {
    return value_;
  }

  DECLARE_EXPRESSION(LiteralExpr);

 public:
  static inline auto New(Datum* value) -> LiteralExpr* {
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
  V(LessThanEqual)

enum BinaryOp : uint64_t {
#define DEFINE_BINARY_OP(Name) k##Name,
  FOR_EACH_BINARY_OP(DEFINE_BINARY_OP)
#undef DEFINE_BINARY_OP
};

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

class BinaryOpExpr : public TemplateExpression<2> {
  static constexpr const auto kLeftInput = 0;
  static constexpr const auto kRightInput = 1;

 private:
  BinaryOp op_;

 protected:
  explicit BinaryOpExpr(const BinaryOp op, Expression* left, Expression* right) :
    TemplateExpression<2>(),
    op_(op) {
    SetLeft(left);
    SetRight(right);
  }

 public:
  ~BinaryOpExpr() override = default;

  auto GetOp() const -> BinaryOp {
    return op_;
  }

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

#define DEFINE_OP_CHECK(Name)                \
  inline auto Is##Name##Op() const -> bool { \
    return GetOp() == BinaryOp::k##Name;     \
  }
  FOR_EACH_BINARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK

  auto IsConstantExpr() const -> bool override;
  auto EvalToConstant() const -> Type* override;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(BinaryOpExpr);

 public:
  static inline auto New(const BinaryOp op, Expression* left, Expression* right) -> BinaryOpExpr* {
    ASSERT(left);
    ASSERT(right);
    return new BinaryOpExpr(op, left, right);
  }

#define DEFINE_NEW_OP(Name)                                                         \
  static inline auto New##Name(Expression* lhs, Expression* rhs) -> BinaryOpExpr* { \
    ASSERT(lhs);                                                                    \
    ASSERT(rhs);                                                                    \
    return New(BinaryOp::k##Name, lhs, rhs);                                        \
  }
  FOR_EACH_BINARY_OP(DEFINE_NEW_OP)
#undef DEFINE_NEW_OP
};

class ConsExpr : public TemplateExpression<2> {
  static constexpr const auto kLeftInput = 0;
  static constexpr const auto kRightInput = 1;

 protected:
  explicit ConsExpr(Expression* left, Expression* right) :
    TemplateExpression<2>() {
    SetCar(left);
    SetCdr(right);
  }

 public:
  ~ConsExpr() override = default;

  auto GetCar() const -> Expression* {
    return GetChildAt(kLeftInput);
  }

  inline auto HasCar() const -> bool {
    return GetCar() != nullptr;
  }

  inline void SetCar(Expression* value) {
    ASSERT(value);
    SetChildAt(kLeftInput, value);
  }

  auto GetCdr() const -> Expression* {
    return GetChildAt(kRightInput);
  }

  inline auto HasCdr() const -> bool {
    return GetCdr() != nullptr;
  }

  inline void SetCdr(Expression* value) {
    ASSERT(value);
    SetChildAt(kRightInput, value);
  }

  auto IsConstantExpr() const -> bool override;
  auto EvalToConstant() const -> Type* override;
  DECLARE_EXPRESSION(ConsExpr);

 public:
  static inline auto New(Expression* car, Expression* cdr) -> ConsExpr* {
    ASSERT(car);
    ASSERT(cdr);
    return new ConsExpr(car, cdr);
  }
};

enum UnaryOp : uint64_t {
  kNot,
  kCar,
  kCdr,
};

static inline auto operator<<(std::ostream& stream, const UnaryOp& rhs) -> std::ostream& {
  switch (rhs) {
    case kCar:
      return stream << "car";
    case kCdr:
      return stream << "cdr";
    default:
      return stream << "Unknown UnaryOp: " << static_cast<uint64_t>(rhs);
  }
}

class UnaryExpr : public TemplateExpression<1> {
 private:
  UnaryOp op_;

 protected:
  UnaryExpr(const UnaryOp op, Expression* value) :
    op_(op) {
    SetChildAt(0, value);
  }

 public:
  ~UnaryExpr() override = default;

  auto GetOp() const -> UnaryOp {
    return op_;
  }

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

  DECLARE_EXPRESSION(UnaryExpr);

 public:
  static inline auto New(const UnaryOp op, Expression* value) -> UnaryExpr* {
    ASSERT(value);
    return new UnaryExpr(op, value);
  }

  static inline auto NewCar(Expression* value) -> UnaryExpr* {
    ASSERT(value);
    return New(kCar, value);
  }

  static inline auto NewCdr(Expression* value) -> UnaryExpr* {
    ASSERT(value);
    return New(kCdr, value);
  }
};

class ThrowExpr : public TemplateExpression<1> {
 protected:
  explicit ThrowExpr(LiteralExpr* value) {
    SetValue(value);
  }

  inline void SetValue(LiteralExpr* expr) {
    ASSERT(expr);
    SetChildAt(0, expr);
  }

 public:
  ~ThrowExpr() override = default;

  inline auto GetValue() const -> LiteralExpr* {
    return GetChildAt(0)->AsLiteralExpr();
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  DECLARE_EXPRESSION(ThrowExpr);

 public:
  static inline auto New(LiteralExpr* value) -> ThrowExpr* {
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
  friend class scm::Parser;
  DEFINE_NON_COPYABLE_TYPE(SequenceExpr);

 private:
  ExpressionList children_{};

 protected:
  SequenceExpr(const ExpressionList& children) {
    children_.insert(std::end(children_), std::begin(children), std::end(children));
  }

 public:
  ~SequenceExpr() override = default;

  auto GetNumberOfChildren() const -> uint64_t override {
    return children_.size();
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
    children_.erase(children_.begin() + idx);
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
  ~BeginExpr() = default;

  DECLARE_EXPRESSION(BeginExpr);

 public:
  static inline auto New(const ExpressionList& expressions = {}) -> BeginExpr* {
    return new BeginExpr(expressions);
  }
};

class EvalExpr : public TemplateExpression<1> {
 protected:
  explicit EvalExpr(Expression* expr) :
    TemplateExpression<1>() {
    SetChildAt(0, expr);
  }

 public:
  ~EvalExpr() = default;

  auto GetExpression() const -> Expression* {
    return GetChildAt(0);
  }

  auto HasExpression() const -> bool {
    return HasChildAt(0);
  }

  DECLARE_EXPRESSION(EvalExpr);

 public:
  static inline auto New(Expression* expr) -> EvalExpr* {
    ASSERT(expr);
    return new EvalExpr(expr);
  }
};

class CallProcExpr : public Expression {
 private:
  Expression* target_ = nullptr;
  ExpressionList args_{};

 protected:
  explicit CallProcExpr(Expression* target, const ExpressionList& args) :
    Expression(),
    args_(args) {
    SetTarget(target);
  }

  inline void SetTarget(Expression* target) {
    ASSERT(target);
    target_ = target;
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

  DECLARE_EXPRESSION(CallProcExpr);

 public:
  static inline auto New(Expression* target, const ExpressionList& args = {}) -> CallProcExpr* {
    return new CallProcExpr(target, args);
  }
};

class CondExpr : public Expression {
 private:
  Expression* test_ = nullptr;
  Expression* conseq_ = nullptr;
  Expression* alt_ = nullptr;

 protected:
  CondExpr(Expression* test, Expression* conseq, Expression* alt) :
    Expression() {
    SetTest(test);
    SetConseq(conseq);
    if (alt)
      SetAlt(alt);
  }

  inline void SetTest(Expression* expr) {
    ASSERT(expr);
    test_ = expr;
  }

  inline void SetConseq(Expression* expr) {
    ASSERT(expr);
    conseq_ = expr;
  }

  inline void SetAlt(Expression* expr) {
    ASSERT(expr);
    alt_ = expr;
  }

 public:
  ~CondExpr() override = default;

  auto GetTest() const -> Expression* {
    return test_;
  }

  auto GetConseq() const -> Expression* {
    return conseq_;
  }

  auto GetAlternate() const -> Expression* {
    return alt_;
  }

  inline auto HasAlternate() const -> bool {
    return GetAlternate() != nullptr;
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(CondExpr);

 public:
  static inline auto New(Expression* test, Expression* conseq, Expression* alt = nullptr) -> CondExpr* {
    ASSERT(test);
    ASSERT(conseq);
    return new CondExpr(test, conseq, alt);
  }
};

class SetExpr : public Expression {
 private:
  Symbol* symbol_;
  Expression* value_;

 protected:
  SetExpr(Symbol* symbol, Expression* value) :
    Expression(),
    symbol_(symbol),
    value_(value) {}

 public:
  ~SetExpr() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
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

  DECLARE_EXPRESSION(SetExpr);

 public:
  static inline auto New(Symbol* symbol, Expression* value) -> SetExpr* {
    ASSERT(symbol);
    ASSERT(value);
    return new SetExpr(symbol, value);
  }
};

class LambdaExpr : public Expression {
  friend class scm::Parser;

 protected:
  ArgumentSet args_;
  Expression* body_;

  explicit LambdaExpr(const ArgumentSet& args, Expression* body) :
    Expression(),
    args_(args),
    body_(body) {}

 public:
  ~LambdaExpr() override = default;

  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetBody() const -> Expression* {
    return body_;
  }

  inline auto HasBody() const -> bool {
    return GetBody() != nullptr;
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(LambdaExpr);

 public:
  static inline auto New(const ArgumentSet& args, Expression* body) -> LambdaExpr* {
    return new LambdaExpr(args, body);
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
  Symbol* symbol_ = nullptr;

  inline void SetSymbol(Symbol* symbol) {
    ASSERT(symbol);
    symbol_ = symbol;
  }

 protected:
  LocalDef(Symbol* symbol, Expression* value) :
    TemplateDefinition<1>() {
    SetSymbol(symbol);
    SetChildAt(0, value);
  }

 public:
  ~LocalDef() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetValue() const -> Expression* {
    return GetChildAt(0);
  }

  auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  DECLARE_EXPRESSION(LocalDef);

 public:
  static inline auto New(Symbol* symbol, Expression* value) -> LocalDef* {
    ASSERT(symbol);
    ASSERT(value);
    return new LocalDef(symbol, value);
  }
};

class ImportDef : public Definition {
 private:
  Symbol* symbol_;

  explicit ImportDef(Symbol* symbol) :
    Definition(),
    symbol_(symbol) {
    ASSERT(symbol_);
  }

 public:
  ~ImportDef() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  DECLARE_EXPRESSION(ImportDef);

 public:
  static inline auto New(Symbol* symbol) -> ImportDef* {
    ASSERT(symbol);
    return new ImportDef(symbol);
  }
};

class ModuleDef : public Definition {
  friend class scm::Parser;

 private:
  Symbol* symbol_ = nullptr;
  DefinitionList definitions_{};

 protected:
  ModuleDef(Symbol* symbol) :
    Definition() {
    SetSymbol(symbol);
  }

  inline void SetSymbol(Symbol* symbol) {
    ASSERT(symbol);
    symbol_ = symbol;
  }

  void SetChildAt(const uint64_t idx, Expression* expr) override {
    ASSERT(idx >= 0 && idx < GetNumberOfChildren())
    ASSERT(expr);
    ASSERT(expr->IsDefinition());
    definitions_[idx] = expr->AsDefinition();
  }

  inline void Append(Definition* defn) {
    ASSERT(defn);
    definitions_.push_back(defn);
  }

 public:
  ~ModuleDef() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return definitions_.size();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return definitions_[idx];
  }

  auto GetDefinitionAt(const uint64_t idx) const -> Definition* {
    return GetChildAt(idx)->AsDefinition();
  }

  auto VisitAllImportDefs(ExpressionVisitor* vis) -> bool;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(ModuleDef);

 public:
  static inline auto New(Symbol* symbol) -> ModuleDef* {
    ASSERT(symbol);
    return new ModuleDef(symbol);
  }
};

class MacroDef : public TemplateDefinition<1> {
 private:
  Symbol* symbol_;

 protected:
  explicit MacroDef(Symbol* symbol, Expression* body) :
    TemplateDefinition<1>(),
    symbol_(symbol) {
    if (body)
      SetBody(body);
  }

  inline void SetBody(Expression* expr) {
    ASSERT(expr);
    SetChildAt(0, expr);
  }

 public:
  ~MacroDef() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  inline auto GetBody() const -> Expression* {
    return GetChildAt(0);
  }

  inline auto HasBody() const -> bool {
    return GetBody() != nullptr;
  }

  DECLARE_EXPRESSION(MacroDef);

 public:
  static inline auto New(Symbol* symbol, Expression* body = nullptr) -> MacroDef* {
    ASSERT(symbol);
    return new MacroDef(symbol, body);
  }
};
}  // namespace expr

using expr::BinaryOp;
using expr::Expression;
using expr::ExpressionList;
using expr::ExpressionVisitor;
#define DEFINE_USE(Name) using expr::Name;
FOR_EACH_EXPRESSION_NODE(DEFINE_USE)
#undef DEFINE_USE
}  // namespace scm

#undef DECLARE_EXPRESSION
#endif  // SCM_EXPRESSION_H
