#include <gflags/gflags.h>
#include <glog/logging.h>
#include <units.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <rpp/sources/fwd.hpp>

#include "gel/common.h"
#include "gel/error.h"
#include "gel/expression.h"
#include "gel/expression_dot.h"
#include "gel/flags.h"
#include "gel/flow_graph_builder.h"
#include "gel/flow_graph_compiler.h"
#include "gel/flow_graph_dot.h"
#include "gel/heap.h"
#include "gel/instruction.h"
#include "gel/local_scope.h"
#include "gel/object.h"
#include "gel/parser.h"
#include "gel/repl.h"
#include "gel/runtime.h"
#include "gel/rx.h"
#include "gel/type_traits.h"
#include "gel/zone.h"

using namespace gel;

struct TimedResult {
  DEFINE_DEFAULT_COPYABLE_TYPE(TimedResult);

 public:
  gel::Object* result;
  Clock::duration duration;

  TimedResult() = default;
  TimedResult(gel::Object* r, const Clock::duration& d) :
    result(r),
    duration(d) {}
  TimedResult(const std::pair<gel::Object*, Clock::duration>& value) :
    result(value.first),
    duration(value.second) {}
  ~TimedResult() = default;

  auto IsError() const -> bool {
    return gel::IsError(result);
  }

  auto IsNull() const -> bool {
    return gel::IsNull(result);
  }

  operator bool() const {
    return !IsError();
  }

  friend auto operator<<(std::ostream& stream, const TimedResult& rhs) -> std::ostream& {
    const auto& result = rhs.result;
    const auto& duration = rhs.duration;
    DLOG(INFO) << "finished in " << units::time::nanosecond_t(static_cast<double>(duration.count()));
    if (gel::IsNull(result))
      return stream;
    if (gel::IsError(result))
      return stream << "error: " << ToError(result)->GetMessage();
    ASSERT(!gel::IsNull(result));
    stream << "result: ";
    PrintValue(stream, result) << std::endl;
    return stream;
  }
};

template <class E, const Severity S = google::INFO>
static inline void DumpFlowGraph(E* value, std::enable_if_t<has_entry<E>::value>* = nullptr) {
  if (!FLAGS_dump_flow_graph)
    return;
  LOG_AT_LEVEL(S) << value << " Instructions:";
  InstructionLogger::Log<S>(value->GetEntry());
}

// TODO: cleanup
static inline auto Execute(const std::string& expr) -> int {
  if (FLAGS_dump_ast) {
    // TODO: implement
  }

  if (FLAGS_dump_ast) {
    try {
      ArgumentSet args{};
      expr::ExpressionList body = {
          Parser::ParseExpr(expr),
      };
      const auto lambda = Lambda::New(args, body);
      LOG_IF(FATAL, !FlowGraphCompiler::Compile(lambda, GetRuntime()->GetScope())) << "failed to compile: " << expr;
      DumpFlowGraph(lambda);
    } catch (const gel::Exception& exc) {
      LOG(ERROR) << "failed to execute expression.";
      std::cerr << " * expression: " << expr << std::endl;
      std::cerr << " * message: " << exc.GetMessage() << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (!FLAGS_eval)
    return EXIT_SUCCESS;
  const TimedResult result = TimedExecution<Object*>([&expr]() -> Object* {
    try {
      return Runtime::Eval(expr);
    } catch (const gel::Exception& exc) {
      return Error::New(fmt::format("failed to execute expression: {}", exc.GetMessage()));
    }
  });
  if (!result) {
    std::cerr << result;
    return EXIT_FAILURE;
  }
  std::cout << result;
  return EXIT_SUCCESS;
}

static inline auto ExecuteScript(const std::string& filename) -> int {
  const auto script = Script::FromFile(filename);
  ASSERT(script);
  if (FLAGS_dump_ast) {
    // TODO: implement
  }
  DumpFlowGraph(script);
  if (!FLAGS_eval)
    return EXIT_SUCCESS;
  const TimedResult result = TimedExecution<Object*>([script]() -> Object* {
    try {
      return Runtime::Exec(script);
    } catch (const gel::Exception& exc) {
      return Error::New(fmt::format("failed to execute script: {}", exc.GetMessage()));
    }
  });
  if (!result) {
    std::cerr << result;
    return EXIT_FAILURE;
  }
  std::cout << result;
  return EXIT_SUCCESS;
}

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  Heap::Init();
  Runtime::Init();
  VLOG(1) << "${GEL_HOME} := " << GetHomeEnvVar();
  const auto expr = GetExpressionFlag();
  if (expr)
    return Execute((*expr));
  if (argc >= 2)
    return ExecuteScript(std::string(argv[1]));
  ASSERT(argc <= 1);
  return Repl::Run();
}