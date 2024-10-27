#include "scheme/flags.h"

namespace scm {
DEFINE_string(reports_dir, "", "Set the reports directory.");
DEFINE_string(expr, "", "Evaluate an expression.");
DEFINE_string(module, "", "Add a module");
DEFINE_bool(dump_ast, false, "Dump a visualiation of the Abstract Syntax Tree (AST)");
DEFINE_bool(dump_flow_graph, false, "Dump a visualization of the Abstract Syntax Tree (AST)");
}  // namespace scm