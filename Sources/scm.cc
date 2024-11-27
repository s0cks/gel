#include <gflags/gflags.h>
#include <glog/logging.h>
#include <units.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

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
#include "scheme/rx.h"
#include "scheme/zone.h"

using namespace scm;

static inline auto PrintTimedResult(Object* result, const Clock::duration& duration) -> int {
  DLOG(INFO) << "finished in " << units::time::nanosecond_t(static_cast<double>(duration.count()));
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

static inline auto Execute(const std::string& expr) -> int {
  if (FLAGS_eval) {
    const auto [result, time] = TimedExecution<Object*>([&expr]() -> Object* {
      try {
        return Runtime::Eval(expr);
      } catch (const scm::Exception& exc) {
        return Error::New(fmt::format("failed to execute expression: {}", exc.GetMessage()));
      }
    });
    return PrintTimedResult(result, time);
  } else if (FLAGS_dump_ast || FLAGS_dump_flow_graph) {
    try {
      const auto expression = ExpressionCompiler::Compile(expr, GetRuntime()->GetGlobalScope());
      ASSERT(expression && expression->HasEntry());
      LOG(INFO) << "result: ";
      LOG_IF(FATAL, !Disassembler::Disassemble(expression->GetEntry())) << "failed to disassemble: " << expression;
    } catch (const scm::Exception& exc) {
      LOG(ERROR) << "failed to execute expression.";
      std::cerr << " * expression: " << expr << std::endl;
      std::cerr << " * message: " << exc.GetMessage() << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

static inline auto ExecuteScript(const std::string& filename) -> int {
  const auto script = Script::FromFile(filename);
  ASSERT(script && script->IsCompiled());
  const auto [result, time] = TimedExecution<Object*>([script]() -> Object* {
    try {
      return Runtime::Exec(script);
    } catch (const scm::Exception& exc) {
      return Error::New(fmt::format("failed to execute script: {}", exc.GetMessage()));
    }
  });
  return PrintTimedResult(result, time);
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