#include <gflags/gflags.h>
#include <glog/logging.h>
#include <units.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "scheme/common.h"
#include "scheme/disassembler.h"
#include "scheme/error.h"
#include "scheme/expression_compiler.h"
#include "scheme/expression_dot.h"
#include "scheme/flags.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/flow_graph_dot.h"
#include "scheme/heap.h"
#include "scheme/local_scope.h"
#include "scheme/object.h"
#include "scheme/parser.h"
#include "scheme/repl.h"
#include "scheme/runtime.h"
#include "scheme/zone.h"

using namespace scm;

static inline auto Execute(const std::string& rhs) -> int {
  if (FLAGS_eval) {
    try {
      const auto result = Runtime::Eval(rhs);
      if (result) {
        std::cout << "result: ";
        PrintValue(std::cout, result) << std::endl;
      }
    } catch (const scm::Exception& exc) {
      LOG(ERROR) << "failed to execute expression.";
      std::cerr << " * expression: " << rhs << std::endl;
      std::cerr << " * message: " << exc.GetMessage() << std::endl;
      return EXIT_FAILURE;
    }
  } else if (FLAGS_dump_ast || FLAGS_dump_flow_graph) {
    try {
      const auto expression = ExpressionCompiler::Compile(rhs, GetRuntime()->GetGlobalScope());
      ASSERT(expression && expression->HasEntry());
      LOG(INFO) << "result: ";
      LOG_IF(FATAL, !Disassembler::Disassemble(expression->GetEntry())) << "failed to disassemble: " << expression;
    } catch (const scm::Exception& exc) {
      LOG(ERROR) << "failed to execute expression.";
      std::cerr << " * expression: " << rhs << std::endl;
      std::cerr << " * message: " << exc.GetMessage() << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

static inline auto ExecuteScript(const std::string& filename) -> int {
  const auto script = Script::FromFile(filename);
  ASSERT(script && script->IsCompiled());
#ifdef SCM_DEBUG
  const auto start_ts = Clock::now();
#endif  // SCM_DEBUG

  Object* result = nullptr;
  try {
    result = Runtime::Exec(script);
  } catch (const scm::Exception& exc) {
    result = Error::New(fmt::format("failed to execute script: {}", exc.GetMessage()));
  }

#ifdef SCM_DEBUG
  const auto stop_ts = Clock::now();
  const auto total_ns = std::chrono::duration_cast<std::chrono::microseconds>(stop_ts - start_ts).count();
  LOG(INFO) << "script executed in " << units::time::microsecond_t(static_cast<double>(total_ns));
#endif  // SCM_DEBUG

  if (IsError(result)) {
    std::cout << "error: " << ToError(result)->GetMessage();
    return EXIT_FAILURE;
  }

  if (!IsNull(result)) {
    std::cout << "result: ";
    PrintValue(std::cout, result) << std::endl;
  }
  return EXIT_SUCCESS;
}

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  Heap::Init();
  Runtime::Init();
  const auto expr = GetExpressionFlag();
  if (expr)
    return Execute((*expr));
  if (argc >= 2)
    return ExecuteScript(std::string(argv[1]));
  ASSERT(argc <= 1);
  return Repl::Run();
}