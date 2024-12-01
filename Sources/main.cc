#include <gflags/gflags.h>
#include <glog/logging.h>
#include <units.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <rpp/sources/fwd.hpp>

#include "gel/disassembler.h"
#include "gel/error.h"
#include "gel/expression_compiler.h"
#include "gel/expression_dot.h"
#include "gel/flags.h"
#include "gel/flow_graph_builder.h"
#include "gel/flow_graph_dot.h"
#include "gel/heap.h"
#include "gel/instruction.h"
#include "gel/local_scope.h"
#include "gel/object.h"
#include "gel/parser.h"
#include "gel/repl.h"
#include "gel/runtime.h"
#include "gel/rx.h"
#include "gel/zone.h"

using namespace gel;

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
      } catch (const gel::Exception& exc) {
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
    } catch (const gel::Exception& exc) {
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
  ASSERT(script);
  if (FLAGS_dump_ast) {
    // TODO: implement
  }
  if (FLAGS_dump_flow_graph) {
    DLOG(INFO) << "Script instructions:";
    instr::InstructionIterator iter(script);
    while (iter.HasNext()) {
      DLOG(INFO) << "- " << iter.Next()->ToString();
    }
  }
  if (FLAGS_eval) {
    const auto [result, time] = TimedExecution<Object*>([script]() -> Object* {
      try {
        return Runtime::Exec(script);
      } catch (const gel::Exception& exc) {
        return Error::New(fmt::format("failed to execute script: {}", exc.GetMessage()));
      }
    });
    return PrintTimedResult(result, time);
  }
  return EXIT_SUCCESS;
}

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  const auto sub = rx::source::from_iterable(std::vector<std::string>{"Hello", "World"});

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