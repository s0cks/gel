#ifndef GEL_EXPRESSION_H
#define GEL_EXPRESSION_H

#include <ostream>
#include <string>
#include <utility>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/hashcode.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/symbol.h"
#include "gel/variable.h"

#define FOR_EACH_EXPRESSION_NODE(V) \
  V(SeqExpr)                        \
  V(LiteralExpr)                    \
  V(UnaryOpExpr)                    \
  V(BinaryOpExpr)                   \
  V(DoExpr)                         \
  V(WhileExpr)                      \
  V(CondExpr)                       \
  V(ClauseExpr)                     \
  V(ImportExpr)                     \
  V(InvokeExpr)                     \
  V(InvokeMacroExpr)                \
  V(InvokeNativeExpr)               \
  V(InvokeInstanceExpr)             \
  V(LoadInstanceMethodExpr)         \
  V(StoreFieldExpr)                 \
  V(StoreLocalExpr)                 \
  V(BindingExpr)                    \
  V(ForeachExpr)                    \
  V(LetExpr)                        \
  V(ThrowExpr)                      \
  V(NewExpr)                        \
  V(LoadFieldExpr)

namespace gel {
class Parser;
class MacroEffectVisitor;
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
  virtual auto Accept(ExpressionVisitor& vis) -> bool = 0;

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

  virtual auto VisitAllDefinitions(ExpressionVisitor& vis) -> bool {
    return true;
  }

  virtual auto VisitChildren(ExpressionVisitor& vis) -> bool {
    return true;
  }

  auto AsExpression() -> Expression* override {
    return this;
  }

  auto GetType() const -> Class* override {
    return GetClass();
  }

  auto GetHashCode() const -> HashCode override {
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return kInvalidHashCode;
  }

  auto Compare(Object* rhs) const -> bool override {
    ASSERT(rhs);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return -1;
  }

  auto Equals(Object* rhs) const -> bool override {
    ASSERT(rhs);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
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
 protected:
  static Class* kClass;

 public:
  static void Init();

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};

template <const uint64_t NumInputs>
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

  auto VisitChildren(ExpressionVisitor& vis) -> bool override {
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
  auto Accept(ExpressionVisitor& vis) -> bool override; \
  auto ToString() const -> std::string override;        \
  auto GetName() const -> const char* override {        \
    return #Name;                                       \
  }                                                     \
  auto As##Name() -> Name* override {                   \
    return this;                                        \
  }

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
}  // namespace expr

using expr::Expression;
using expr::ExpressionList;
using expr::ExpressionVisitor;
#define DEFINE_USE(Name) using expr::Name;
FOR_EACH_EXPRESSION_NODE(DEFINE_USE)
#undef DEFINE_USE
}  // namespace gel

#endif  // GEL_EXPRESSION_H
