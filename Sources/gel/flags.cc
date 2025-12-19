#include "flags.h"

#include <gflags/gflags.h>

namespace gel {
DEFINE_string(reports_dir, "", "Set the reports directory.");
DEFINE_string(expr, "", "Evaluate an expression.");
DEFINE_string(module, "", "Add a module");
DEFINE_bool(eval, true, "Enable expression evaluation");

#ifdef GEL_ENABLE_GRAPHVIZ
DEFINE_bool(dump_ast, false, "Dump a visualiation of the Abstract Syntax Tree (AST)");
DEFINE_bool(dump_flow_graph, false, "Dump a visualization of the Abstract Syntax Tree (AST)");
#endif  // GEL_ENABLE_GRAPHVIZ

DEFINE_bool(pedantic, true, "Enable/disable pedantic compilation.");
DEFINE_bool(print_ir, false, "Print the IR after building the Control Flow Graph (CFG)");
DEFINE_bool(print_bytecode, false, "Print compiled bytecode.");
}  // namespace gel
