#include "scheme/gv.h"

#include <glog/logging.h>

namespace scm::dot {
auto Graph::New(GraphBuilder* builder) -> Graph* {
  ASSERT(builder);
  return new Graph(builder->GetGraph());
}

void GraphRenderer::RenderDotTo(Graph* graph, FILE* stream) {
  ASSERT(HasContext());
  ASSERT(graph);
  ASSERT(stream);
  return RenderTo(graph, stream, "dot", "dot");
}

void GraphRenderer::RenderTo(Graph* graph, FILE* stream, const std::string& layout, const std::string& format) {
  ASSERT(HasContext());
  ASSERT(stream);
  ASSERT(graph);
  ASSERT(!layout.empty());
  ASSERT(!format.empty());
  gvLayout(GetContext(), graph->get(), layout.c_str());
  gvRender(GetContext(), graph->get(), format.c_str(), stream);
  gvFreeLayout(GetContext(), graph->get());
}

void Graph::RenderTo(FILE* stream) {
  ASSERT(stream);
  GraphRenderer render;
  render.RenderDotTo(this, stream);
}

void Graph::RenderPngTo(FILE* stream) {
  ASSERT(stream);
  GraphRenderer render;
  render.RenderPngTo(this, stream);
}

void Graph::RenderPngToFilename(const std::string& filename) {
  ASSERT(!filename.empty());
  const auto file = fopen(filename.c_str(), "wb");
  LOG_IF(FATAL, !file) << "failed to open: " << filename;
  ASSERT(file);
  RenderPngTo(file);
  const auto result = fclose(file);
  LOG_IF(FATAL, result != 0) << "failed to close: " << filename;
}
}  // namespace scm::dot