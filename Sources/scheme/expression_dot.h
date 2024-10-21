#ifndef SCM_EXPRESSION_DOT_H
#define SCM_EXPRESSION_DOT_H

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/gv.h"

namespace scm {
namespace expr {
class ExpressionToDot : public dot::GraphBuilder, public ExpressionVisitor {
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
  EdgeList edges_{};

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
    edges_.push_back(edge);
  }

 public:
  explicit ExpressionToDot(const char* graph_name);
  ~ExpressionToDot() override = default;

  auto Build() -> dot::Graph* override;
#define DEFINE_VISIT(Name) auto Visit##Name(Name##Expr* expr) -> bool override;
  FOR_EACH_EXPRESSION_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
 public:
  static auto BuildGraph(const char* name, Expression* expr) -> dot::Graph* {
    ASSERT(name);
    ASSERT(expr);
    ExpressionToDot builder(name);
    if (!expr->Accept(&builder)) {
      DLOG(ERROR) << "failed to visit: " << expr->ToString();
      return nullptr;
    }
    return builder.Build();
  }
};
}  // namespace expr
using namespace expr;
}  // namespace scm

#endif  // SCM_EXPRESSION_DOT_H
