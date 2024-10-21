#ifndef SCM_GV_H
#define SCM_GV_H

#include <fmt/format.h>
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <graphviz/gvcext.h>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "scheme/common.h"

namespace scm::dot {
using Symbol = Agsym_t;
class Graph {
  using Handle = Agraph_t;
  DEFINE_NON_COPYABLE_TYPE(Graph);

 private:
  Handle* handle_ = nullptr;

  explicit Graph(Handle* handle) {
    SetHandle(handle);
  }

  inline void SetHandle(Handle* handle) {
    ASSERT(handle);
    handle_ = handle;
  }

 public:
  ~Graph();

  auto get() const -> Handle* {
    return handle_;
  }

  void RenderTo(FILE* stream);
  void RenderPngTo(FILE* stream);
  void RenderPngToFilename(const std::string& filename);

  inline void RenderToStdout() {
    return RenderTo(stdout);
  }

 public:
  static inline auto New(Handle* handle) -> Graph* {
    return new Graph(handle);
  }

  static inline auto New(const char* name, Agdesc_t desc, Agdisc_t* disc = nullptr) -> Graph* {
    return New(agopen(const_cast<char*>(name), desc, disc));  // NOLINT
  }

  static inline auto New(const std::string& name, Agdesc_t desc, Agdisc_t* disc = nullptr) -> Graph* {
    return New(name.c_str(), desc, disc);
  }
};

class GraphBuilder {
  DEFINE_NON_COPYABLE_TYPE(GraphBuilder);

 public:
  static constexpr const auto kDefaultEdgeFlags = 1;
  static constexpr const auto kDefaultNodeFlags = 1;
  using Node = Agnode_t;
  using NodeList = std::vector<Node*>;
  using Edge = Agedge_t;
  using EdgeList = std::vector<Edge*>;

 private:
  Graph* graph_ = nullptr;
  uint64_t num_nodes_ = 0;

  inline auto NextNodeId() -> std::string {
    return fmt::format("n{0:d}", ++num_nodes_);
  }

 protected:
  GraphBuilder(const char* name, Agdesc_t desc);

  inline void SetGraph(Graph* g) {
    ASSERT(g);
    graph_ = g;
  }

  inline auto GetGraph() const -> Graph* {
    return graph_;
  }

  inline auto HasGraph() const -> bool {
    return GetGraph() != nullptr;
  }

  inline void SetAttr(int kind, const std::string& name, const std::string& value) {
    agattr(GetGraph()->get(), kind, const_cast<char*>(name.c_str()), const_cast<char*>(value.c_str()));  // NOLINT
  }

  inline void SetNodeAttr(const std::string& name, const std::string& value) {
    return SetAttr(AGNODE, name, value);
  }

  inline void SetGraphAttr(const std::string& name, const std::string& value) {
    return SetAttr(AGRAPH, name, value);
  }

  inline void SetEdgeAttr(const std::string& name, const std::string& value) {
    return SetAttr(AGEDGE, name, value);
  }

  inline auto NewNode(const char* name, const int flags = kDefaultNodeFlags) -> Node* {
    ASSERT(HasGraph());
    ASSERT(name);
    return agnode(GetGraph()->get(), const_cast<char*>(name), flags);
  }

  inline auto NewNode(const std::string& name, const int flags = kDefaultNodeFlags) -> Node* {
    ASSERT(HasGraph());
    ASSERT(!name.empty());
    return NewNode(name.c_str(), flags);
  }

  inline auto NewNode(const int flags = kDefaultNodeFlags) -> Node* {
    return NewNode(NextNodeId(), flags);
  }

  inline auto NewEdge(Node* from, Node* to, const std::string& name = "", const int flags = kDefaultEdgeFlags) -> Edge* {
    ASSERT(from);
    ASSERT(to);
    return agedge(GetGraph()->get(), from, to, const_cast<char*>(name.c_str()), flags);  // NOLINT
  }

  inline void Set(Node* node, const char* name, const char* value) {
    ASSERT(node);
    ASSERT(name);
    ASSERT(strlen(name) > 0);
    ASSERT(value);
    ASSERT(strlen(value) > 0);
    agset(node, const_cast<char*>(name), const_cast<char*>(value));
  }

  inline void SetNodeLabel(Node* node, const char* value) {
    static constexpr const auto kKey = "label";
    return Set(node, kKey, value);
  }

  inline void SetNodeLabel(Node* node, const std::string& value) {
    return SetNodeLabel(node, value.c_str());
  }

  inline void SetNodeLabel(Node* node, const std::stringstream& value) {
    return SetNodeLabel(node, value.str());
  }

  inline void SetNodeXLabel(Node* node, const char* value) {
    static constexpr const auto kKey = "xlabel";
    return Set(node, kKey, value);
  }

  inline void SetNodeXLabel(Node* node, const std::string& value) {
    return SetNodeXLabel(node, value.c_str());
  }

  inline void SetNodeXLabel(Node* node, const std::stringstream& value) {
    return SetNodeXLabel(node, value.str());
  }

 public:
  virtual ~GraphBuilder() = default;
  virtual auto Build() -> Graph* = 0;
};

class GraphRenderer {
  using Context = GVC_t;
  DEFINE_NON_COPYABLE_TYPE(GraphRenderer);

 private:
  Context* context_ = nullptr;

  void SetContext(Context* ctx) {
    ASSERT(ctx);
    context_ = ctx;
  }

  auto GetContext() const -> Context* {
    return context_;
  }

  auto HasContext() const -> bool {
    return GetContext() != nullptr;
  }

 private:
  static inline auto NewContext() -> Context* {
    return gvContext();
  }

  static inline void DeleteContext(Context* ctx) {
    ASSERT(ctx);
    gvFreeContext(ctx);
  }

 public:
  GraphRenderer() {
    SetContext(NewContext());
  }
  ~GraphRenderer() {
    if (HasContext())
      DeleteContext(GetContext());
  }

  void RenderTo(Graph* graph, FILE* stream, const std::string& layout, const std::string& format);
  void RenderDotTo(Graph* graph, FILE* stream);

  inline void RenderDotToStdout(Graph* graph) {
    return RenderDotTo(graph, stdout);
  }

  inline void RenderPngTo(Graph* graph, FILE* stream, const std::string& layout = "dot") {
    ASSERT(stream);
    ASSERT(graph);
    ASSERT(!layout.empty());
    return RenderTo(graph, stream, layout, "png");
  }
};
}  // namespace scm::dot

#endif  // SCM_GV_H
