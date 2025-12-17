#include <chrono>
#include <cstdlib>
#include <fmt/format.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <string>
#include <units.h>
#include <utility>

#include "gel/common.h"
#include "gel/type/type_traits.h"

using namespace gel;

// TODO: cleanup
// static inline auto Execute(const std::string& expr) -> int {
//   if (FLAGS_dump_ast) {
//     try {
//       const auto lambda = Parser::ParseExpr(expr);
//       LOG_IF(FATAL, !FlowGraphCompiler::Compile(*lambda, GetRuntime()->GetScope())) << "failed to compile: " << expr;
//     } catch (const gel::Exception& exc) {
//       LOG(ERROR) << "failed to execute expression.";
//       std::cerr << " * expression: " << expr << std::endl;
//       std::cerr << " * message: " << exc.GetMessage() << std::endl;
//       return EXIT_FAILURE;
//     }
//   }
//
//   if (!FLAGS_eval)
//     return EXIT_SUCCESS;
//   const TimedResult result = TimedExecution<Object*>([&expr]() -> Object* {
//     try {
//       return Runtime::Eval(expr);
//     } catch (const gel::Exception& exc) {
//       return Error::New(fmt::format("failed to execute expression: {}", exc.GetMessage()));
//     }
//   });
//   if (!result) {
//     std::cerr << result;
//     return EXIT_FAILURE;
//   }
//   std::cout << result;
//   return EXIT_SUCCESS;
// }
//
// static inline auto ExecuteScript(const std::string& filename) -> int {
//   const auto script = Script::FromFile(filename);
//   ASSERT(script);
//   if (!FLAGS_eval)
//     return EXIT_SUCCESS;
//   const TimedResult result = TimedExecution<Object*>([script]() -> Object* {
//     try {
//       return Runtime::Exec(script);
//     } catch (const gel::Exception& exc) {
//       return Error::New(fmt::format("failed to execute script: {}", exc.GetMessage()));
//     }
//   });
//   if (!result) {
//     std::cerr << result;
//     return EXIT_FAILURE;
//   }
//   std::cout << result;
//   return EXIT_SUCCESS;
// }

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  // Parser::Init();
  // Heap::Init();
  // Runtime::Init();

  int result = EXIT_FAILURE;
  // const auto expr = GetExpressionFlag();
  // if (expr) {
  //   result = Execute((*expr));
  // } else if (argc >= 2) {
  //   result = ExecuteScript(std::string(argv[1]));
  // } else {
  //   ASSERT(argc <= 1);
  //   result = Repl::Run();
  // }
  // GetRuntime()->Shutdown();
  return result;
}
