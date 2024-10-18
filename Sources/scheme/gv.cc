#include "scheme/gv.h"

#include <glog/logging.h>
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>

namespace scm {
DotGraph::DotGraph(Agraph_t* graph) :
  graph_(graph) {
  ASSERT(graph);
}

DotGraph::~DotGraph() {
  agclose(graph_);
  graph_ = nullptr;
}

auto DotGraph::New(Agraph_t* graph) -> DotGraph* {
  return new DotGraph(graph);
}

DotGraphBuilder::DotGraphBuilder(const char* name, Agdesc_t desc) :
  graph_(nullptr) {
  const auto graph = agopen(const_cast<char*>(name), desc, nullptr);
  ASSERT(graph);
  graph_ = graph;
}

DotGraphRenderer::DotGraphRenderer() :
  context_(nullptr) {
  SetContext(gvContext());
}

DotGraphRenderer::~DotGraphRenderer() {
  if (HasContext())
    gvFreeContext(GetContext());
}

void DotGraphRenderer::RenderDotTo(DotGraph* graph, FILE* stream) {
  ASSERT(HasContext());
  ASSERT(graph);
  ASSERT(stream);
  return RenderTo(graph, stream, "dot", "dot");
}

void DotGraphRenderer::RenderTo(DotGraph* graph, FILE* stream, const std::string& layout, const std::string& format) {
  ASSERT(HasContext());
  ASSERT(stream);
  ASSERT(graph);
  ASSERT(!layout.empty());
  ASSERT(!format.empty());
  gvLayout(GetContext(), graph->get(), layout.c_str());
  gvRender(GetContext(), graph->get(), format.c_str(), stream);
  gvFreeLayout(GetContext(), graph->get());
}

void DotGraph::RenderTo(FILE* stream) {
  ASSERT(stream);
  DotGraphRenderer render;
  render.RenderDotTo(this, stream);
}

void DotGraph::RenderPngTo(FILE* stream) {
  ASSERT(stream);
  DotGraphRenderer render;
  render.RenderPngTo(this, stream);
}

void DotGraph::RenderPngToFilename(const std::string& filename) {
  ASSERT(!filename.empty());
  const auto file = fopen(filename.c_str(), "wb");
  LOG_IF(FATAL, !file) << "failed to open: " << filename;
  ASSERT(file);
  RenderPngTo(file);
  const auto result = fclose(file);
  LOG_IF(FATAL, result != 0) << "failed to close: " << filename;
}
}  // namespace scm