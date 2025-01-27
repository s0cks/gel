#ifndef GEL_EXPRESSION_DOT_H
#define GEL_EXPRESSION_DOT_H
#ifdef GEL_ENABLE_GV

#include <glog/logging.h>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/gv.h"

namespace gel {
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
      if (GetOwner()->HasParent())
        SetPrevious(GetOwner()->GetParent());
      GetOwner()->SetParent(current);
    }
    ~NodeScope() {
      if (HasOwner() && HasPrevious())
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

  inline auto NextNodeId() -> std::string {
    return fmt::format("e{0:d}", num_expressions_++);
  }

  inline auto NewNode() -> Node* {
    const auto node_id = NextNodeId();
    return dot::GraphBuilder::NewNode(node_id);
  }

 private:
  Node* parent_ = nullptr;
  EdgeList edges_{};
  uint64_t num_expressions_ = 0;

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
    const auto edge = NewEdge(GetParent(), node, fmt::format("e{0:d}", edges_.size()).c_str());
    ASSERT(edge);
    edges_.push_back(edge);
  }

 public:
  explicit ExpressionToDot(const char* graph_name);
  ~ExpressionToDot() override = default;

  auto Build() -> dot::Graph* override;
#define DEFINE_VISIT(Name) auto Visit##Name(Name* expr)->bool override;
  FOR_EACH_EXPRESSION_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
 public:
  static inline auto BuildGraph(const char* name, Expression* expr) -> dot::Graph* {
    ASSERT(name);
    ASSERT(expr);
    ExpressionToDot builder(name);
    if (!expr->Accept(&builder)) {
      DLOG(ERROR) << "failed to visit: " << expr->ToString();
      return nullptr;
    }
    return builder.Build();
  }

  static inline auto BuildGraph(const std::string& name, Expression* expr) -> dot::Graph* {
    ASSERT(!name.empty());
    ASSERT(expr);
    return BuildGraph(name.c_str(), expr);
  }

  static inline auto BuildGraph(Symbol* symbol, Expression* expr) -> dot::Graph* {
    ASSERT(symbol);
    ASSERT(expr);
    return BuildGraph(symbol->Get(), expr);
  }
};
}  // namespace expr
using namespace expr;
}  // namespace gel

#endif  // GEL_ENABLE_GV
#endif  // GEL_EXPRESSION_DOT_H