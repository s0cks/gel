#include "gel/script.h"

#include <fstream>
#include <units.h>

#include "gel/common.h"
#include "gel/flags.h"
#include "gel/hashcode.h"
#include "gel/to_string_helper.h"

#include "gel/frontend/flow_graph_builder.h"
#include "gel/frontend/flow_graph_compiler.h"
#include "gel/frontend/flow_graph_dot.h"
#include "gel/frontend/expr/expression_dot.h"
#include "gel/frontend/parser.h"

#include "gel/type/type.h"
#include "gel/type/macro_fn.h"
#include "gel/type/namespace.h"

namespace gel {
auto Script::New(const ObjectList& args) -> Script* {
  NOT_IMPLEMENTED(FATAL);
}

auto Script::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Script");
}

auto Script::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Script::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsScript())
    return false;
  const auto other = rhs->AsScript();
  ASSERT(other);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Script::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return kInvalidHashCode;
}

auto Script::ToString() const -> std::string {
  ToStringHelper<Script> helper{};
  helper.AddField("scope", scope_);
  helper.AddField("name", name_);
  // helper.AddField("macros", macros_);
  // helper.AddField("lambdas", lambdas_);
  // helper.AddField("namespaces", namespaces_);
  helper.AddField("body", body_);
  helper.AddField("code", code_);
  return helper;
}

auto Script::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!Visit(name_, *vis))
    return false;
  if (!Visit(body_, *vis))
    return false;
  // TODO: visit code_
  if (!VisitAll(macros_, *vis))
    return false;
  if (!VisitAll(macros_, *vis))
    return false;
  if (!VisitAll(lambdas_, *vis))
    return false;
  if (!VisitAll(namespaces_, *vis))
    return false;
  return true;
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
    LOG_IF(FATAL, !FlowGraphCompiler::Compile(*script, scope)) << "failed to compile: " << script;
  return script;
}
}  // namespace gel
