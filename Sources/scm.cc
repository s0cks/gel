#include <gflags/gflags.h>
#include <glog/logging.h>

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
#include "scheme/parser.h"
#include "scheme/repl.h"
#include "scheme/runtime.h"

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

class PointerPrinter : public PointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(PointerPrinter);

 private:
  uword count_ = 0;

 public:
  PointerPrinter() = default;
  ~PointerPrinter() override = default;

  auto Visit(Pointer* ptr) -> bool override {
    ASSERT(ptr);
    DLOG(INFO) << "- #" << (++count_) << " " << (*ptr);
    return true;
  }
};

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  Heap::Init();
  const auto heap = Heap::GetHeap();
  ASSERT(heap);
  for (auto x = 0; x < 100; x++) {
    const auto ptr = (uword*)heap->TryAllocate(sizeof(uword));
    ASSERT(ptr);
    (*ptr) = x;
  }

  PointerPrinter printer;
  DLOG(INFO) << "pointers:";
  LOG_IF(FATAL, !heap->GetNewZone().VisitAllPointers(&printer)) << "failed to visit Pointers in: " << heap->GetNewZone();

  PrintHeap(*heap);

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

    std::stringstream code_copy;
    code_copy << code.rdbuf();
    const auto ss = Parser::ParseScript(code_copy);
    ASSERT(ss);

    ScriptCompiler::Compile(ss);
    ASSERT(ss && ss->IsCompiled());

    try {
      const auto result = Runtime::Exec(ss);
      if (result) {
        std::cout << "result: ";
        PrintValue(std::cout, result) << std::endl;
      }
      return EXIT_SUCCESS;
    } catch (const scm::Exception& exc) {
      LOG(ERROR) << "failed to execute compiled script:";
      std::cerr << " * message: " << exc.GetMessage() << std::endl;
      return EXIT_FAILURE;
    }
  }
  return Repl::Run();
}