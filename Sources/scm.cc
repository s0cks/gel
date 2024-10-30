#include <gflags/gflags.h>
#include <glog/logging.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "scheme/common.h"
#include "scheme/error.h"
#include "scheme/expression_compiler.h"
#include "scheme/expression_dot.h"
#include "scheme/flags.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/flow_graph_dot.h"
#include "scheme/module_compiler.h"
#include "scheme/parser.h"
#include "scheme/runtime.h"

using namespace scm;

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  Runtime::Init();
  const auto expr = GetExpressionFlag();
  if (expr && FLAGS_eval) {
    try {
      const auto result = Runtime::Eval((*expr));
      ASSERT(result);
      PrintValue(std::cout, result) << std::endl;
    } catch (const scm::Exception& exc) {
      LOG(ERROR) << "failed to execute expression.";
      std::cerr << " * expression: " << (*expr) << std::endl;
      std::cerr << " * message: " << exc.GetMessage() << std::endl;
      return EXIT_FAILURE;
    }
  } else if (expr && (FLAGS_dump_ast || FLAGS_dump_flow_graph)) {
    try {
      const auto expression = ExpressionCompiler::Compile((*expr));
      ASSERT(expression);
      LOG(INFO) << "result: " << expression;
    } catch (const scm::Exception& exc) {
      LOG(ERROR) << "failed to execute expression.";
      std::cerr << " * expression: " << (*expr) << std::endl;
      std::cerr << " * message: " << exc.GetMessage() << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}