#include "gel/script.h"

#include <units.h>

#include <fstream>

#include "gel/common.h"
#include "gel/expression_dot.h"
#include "gel/flags.h"
#include "gel/flow_graph_builder.h"
#include "gel/flow_graph_dot.h"
#include "gel/lambda.h"
#include "gel/namespace.h"
#include "gel/parser.h"

namespace gel {
auto Script::New(const ObjectList& args) -> Script* {
  NOT_IMPLEMENTED(FATAL);
}

auto Script::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Script");
}

void Script::Append(Lambda* lambda) {
  ASSERT(lambda);
  lambdas_.push_back(lambda);
  lambda->SetOwner(this);
}

void Script::Append(Namespace* ns) {
  ASSERT(ns);
  namespaces_.push_back(ns);
  ns->SetOwner(this);
}

auto Script::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsScript())
    return false;
  const auto other = rhs->AsScript();
  ASSERT(other);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Script::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Script(";
  ss << "scope=" << GetScope();
  ss << ")";
  return ss.str();
}

auto Script::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

void ScriptCompiler::CompileScript(Script* script) {
  ASSERT(script && !script->IsCompiled());
#ifdef GEL_DEBUG
  DVLOG(GEL_VLEVEL_1) << "compiling: " << script->ToString();
  using Clock = std::chrono::high_resolution_clock;
  const auto start = Clock::now();
#endif  // GEL_DEBUG

#ifdef GEL_DEBUG
  if (FLAGS_dump_ast) {
    // TODO: dump script ast
  }
#endif  // GEL_DEBUG

  const auto flow_graph = FlowGraphBuilder::Build(script, script->GetScope());
  ASSERT(flow_graph);
  ASSERT(flow_graph->HasEntry());

#if defined(GEL_DEBUG) && defined(GEL_ENABLE_GV)
  if (FLAGS_dump_flow_graph) {
    const auto dotgraph = FlowGraphToDotGraph::BuildGraph("expr", flow_graph);
    ASSERT(dotgraph);
    dotgraph->RenderPngToFilename(GetReportFilename("exec_expr_flow_graph.png"));
  }
#endif  // defined(GEL_DEBUG) && defined(GEL_ENABLE_GV)

#ifdef GEL_DEBUG
  const auto stop = Clock::now();
  const auto total_ns = std::chrono::duration_cast<std::chrono::milliseconds>((stop - start)).count();
  DVLOG(GEL_VLEVEL_1) << "script compiled in " << units::time::millisecond_t(total_ns);  // NOLINT
#endif                                                                                   // GEL_DEBUG
  script->SetEntry(flow_graph->GetEntry());
  // TODO: delete flow_graph
}

auto Script::FromFile(const std::string& filename, const bool compile) -> Script* {
  DVLOG(10) << "loading script from: " << filename;
  std::stringstream code;
  {
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    LOG_IF(FATAL, !file) << "failed to load script from: " << filename;
    code << file.rdbuf();
    file.close();
  }
  const auto script = Parser::ParseScript(code);
  ASSERT(script);
  if (compile)
    ScriptCompiler::Compile(script);
  return script;
}
}  // namespace gel