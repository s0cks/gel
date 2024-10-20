#ifndef SCM_EXPRESSION_DOT_H
#define SCM_EXPRESSION_DOT_H

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/gv.h"

namespace scm {
namespace expr {
class ExpressionToDot : public DotGraphBuilder, public ExpressionVisitor {
  friend class NodeScope;
  DEFINE_NON_COPYABLE_TYPE(ExpressionToDot);

 private:
  class NodeScope {
    DEFINE_NON_COPYABLE_TYPE(NodeScope);

   private:
    ExpressionToDot* owner_ = nullptr;
    Node* previous_ = nullptr;
    Node* current_ = nullptr;

    inline void SetOwner(ExpressionToDot* owner) {
      ASSERT(owner);
      owner_ = owner;
    }

    inline void SetPrevious(Node* node) {
      ASSERT(node);
      previous_ = node;
    }

    inline void SetCurrent(Node* node) {
      ASSERT(node);
      current_ = node;
    }

   public:
    explicit NodeScope(ExpressionToDot* owner, Node* current) {
      SetOwner(owner);
      if (!HasOwner())
        return;
      SetCurrent(current);
      SetPrevious(GetOwner()->GetParent());
      GetOwner()->SetParent(current);
    }
    ~NodeScope() {
      if (HasOwner())
        GetOwner()->SetParent(GetPrevious());
    }

    auto GetOwner() const -> ExpressionToDot* {
      return owner_;
    }

    auto HasOwner() const -> bool {
      return GetOwner() != nullptr;
    }

    auto GetPrevious() const -> Node* {
      return previous_;
    }

    auto HasPrevious() const -> bool {
      return GetPrevious() != nullptr;
    }
  };

 private:
  Node* parent_ = nullptr;

  inline void SetParent(Node* node) {
    ASSERT(node);
    parent_ = node;
  }

  inline auto GetParent() const -> Node* {
    return parent_;
  }

  inline auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  inline void CreateEdgeFromParent(Node* node) {
    if (!HasParent())
      return;
    ASSERT(node);
    const auto edge = NewEdge(GetParent(), node);
    ASSERT(edge);
  }

 public:
  explicit ExpressionToDot(const char* graph_name);
  ~ExpressionToDot() override = default;

  auto BuildDotGraph() -> DotGraph* override;
#define DEFINE_VISIT(Name) auto Visit##Name(Name##Expr* expr) -> bool override;
  FOR_EACH_EXPRESSION_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
 public:
  static auto Build(const char* name, Expression* expr) -> DotGraph*;
};
}  // namespace expr
using namespace expr;
}  // namespace scm

#endif  // SCM_EXPRESSION_DOT_H
