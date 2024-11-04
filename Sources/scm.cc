#include <gflags/gflags.h>
#include <glog/logging.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
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
#include "scheme/repl.h"
#include "scheme/runtime.h"

using namespace scm;

static inline auto Execute(const std::string& rhs) -> int {
  if (FLAGS_eval) {
    try {
      const auto result = Runtime::Eval(rhs);
      if (result)
        PrintValue(std::cout, result) << std::endl;
    } catch (const scm::Exception& exc) {
      LOG(ERROR) << "failed to execute expression.";
      std::cerr << " * expression: " << rhs << std::endl;
      std::cerr << " * message: " << exc.GetMessage() << std::endl;
      return EXIT_FAILURE;
    }
  } else if (FLAGS_dump_ast || FLAGS_dump_flow_graph) {
    try {
      const auto expression = ExpressionCompiler::Compile(rhs);
      ASSERT(expression);
      LOG(INFO) << "result: " << expression;
    } catch (const scm::Exception& exc) {
      LOG(ERROR) << "failed to execute expression.";
      std::cerr << " * expression: " << rhs << std::endl;
      std::cerr << " * message: " << exc.GetMessage() << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  Runtime::Init();
  const auto expr = GetExpressionFlag();
  if (expr)
    return Execute((*expr));

  if (argc >= 2) {
    const auto script = std::string(argv[1]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    DVLOG(10) << "loading script from: " << script;
    std::stringstream code;
    {
      std::ifstream file(script, std::ios::binary | std::ios::in);
      LOG_IF(FATAL, !file) << "failed to load script from: " << script;
      code << file.rdbuf();
      file.close();
    }

    DVLOG(100) << "code:" << std::endl << code.str();
    return Execute(code.str());
  }
  return Repl::Run();
}