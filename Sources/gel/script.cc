#include "gel/script.h"

#include <units.h>

#include <fstream>

#include "gel/common.h"
#include "gel/expression_dot.h"
#include "gel/flags.h"
#include "gel/flow_graph_builder.h"
#include "gel/flow_graph_compiler.h"
#include "gel/flow_graph_dot.h"
#include "gel/lambda.h"
#include "gel/macro.h"
#include "gel/namespace.h"
#include "gel/parser.h"

namespace gel {
auto Script::New(const ObjectList& args) -> Script* {
  NOT_IMPLEMENTED(FATAL);
}

auto Script::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Script");
}

void Script::Append(Macro* macro) {
  ASSERT(macro);
  macros_.push_back(macro);
  macro->SetOwner(this);
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

auto Script::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
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
  const auto scope = GetRuntime()->GetScope();
  ASSERT(scope);
  if (compile)
    LOG_IF(FATAL, !FlowGraphCompiler::Compile(script, scope)) << "failed to compile: " << script;
  return script;
}
}  // namespace gel