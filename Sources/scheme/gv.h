#ifndef SCM_GV_H
#define SCM_GV_H

#include <graphviz/cgraph.h>
#include <graphviz/gvcext.h>

#include <string>
#include <utility>

#include "scheme/common.h"

namespace scm {
class DotGraph {
  using Symbol = Agsym_t;
  DEFINE_NON_COPYABLE_TYPE(DotGraph);

 private:
  static Symbol* kNodeLabelAttr;

 private:
  Agraph_t* graph_;

  explicit DotGraph(Agraph_t* graph);

 public:
  ~DotGraph();

  auto get() const -> Agraph_t* {
    return graph_;
  }

  void RenderTo(FILE* stream);
  void RenderPngTo(FILE* stream);
  void RenderPngToFilename(const std::string& filename);

  inline void RenderToStdout() {
    return RenderTo(stdout);
  }

 public:
  static void Init();
  static auto New(Agraph_t* graph) -> DotGraph*;
};

class DotGraphBuilder {
  DEFINE_NON_COPYABLE_TYPE(DotGraphBuilder);

 protected:
  Agraph_t* graph_;  // TODO: memory leak

  DotGraphBuilder(const char* name, Agdesc_t desc);

  inline void SetGraph(Agraph_t* g) {
    ASSERT(g);
    graph_ = g;
  }

  inline auto GetGraph() const -> Agraph_t* {
    return graph_;
  }

  inline auto HasGraph() const -> bool {
    return GetGraph() != nullptr;
  }

 public:
  virtual ~DotGraphBuilder() = default;
  virtual auto BuildDotGraph() -> DotGraph* = 0;
};

class DotGraphRenderer {
  DEFINE_NON_COPYABLE_TYPE(DotGraphRenderer);

 private:
  GVC_t* context_;

  void SetContext(GVC_t* ctx) {
    ASSERT(ctx);
    context_ = ctx;
  }

  auto GetContext() const -> GVC_t* {
    return context_;
  }

  auto HasContext() const -> bool {
    return GetContext() != nullptr;
  }

 public:
  DotGraphRenderer();
  ~DotGraphRenderer();
  void RenderTo(DotGraph* graph, FILE* stream, const std::string& layout, const std::string& format);
  void RenderDotTo(DotGraph* graph, FILE* stream);

  inline void RenderDotToStdout(DotGraph* graph) {
    return RenderDotTo(graph, stdout);
  }

  inline void RenderPngTo(DotGraph* graph, FILE* stream, const std::string& layout = "dot") {
    ASSERT(stream);
    ASSERT(graph);
    ASSERT(!layout.empty());
    return RenderTo(graph, stream, layout, "png");
  }
};
}  // namespace scm

#endif  // SCM_GV_H
