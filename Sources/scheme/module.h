#ifndef SCM_MODULE_H
#define SCM_MODULE_H

#include <glog/logging.h>

#include <string>

#include "scheme/common.h"
#include "scheme/lambda.h"
#include "scheme/local.h"
#include "scheme/local_scope.h"
#include "scheme/type.h"

namespace scm {
namespace expr {
class Expression;
}

class Module {
  DEFINE_NON_COPYABLE_TYPE(Module);

 private:
  Symbol* symbol_ = nullptr;
  LocalScope* scope_ = nullptr;
  expr::Expression* body_ = nullptr;

  explicit Module(Symbol* symbol, LocalScope* scope, expr::Expression* body) {
    SetSymbol(symbol);
    SetScope(scope);
    SetBody(body);
  }

  inline void SetSymbol(Symbol* symbol) {
    ASSERT(symbol);
    symbol_ = symbol;
  }

  inline void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

  inline void SetBody(expr::Expression* body) {
    body_ = body;
  }

 public:
  ~Module();

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetBody() const -> expr::Expression* {
    return body_;
  }

  inline auto HasBody() const -> bool {
    return GetBody() != nullptr;
  }

 public:
  static inline auto New(Symbol* symbol, LocalScope* scope = LocalScope::New(), expr::Expression* body = nullptr) -> Module* {
    ASSERT(symbol);
    ASSERT(scope);
    return new Module(symbol, scope, body);
  }
};

class ModulePrinter : public LocalVariableVisitor {
  DEFINE_NON_COPYABLE_TYPE(ModulePrinter);

 private:
  std::string file_;
  int line_;
  google::LogSeverity severity_;
  int indent_ = 0;

  inline void Indent() {
    indent_ += 1;
  }

  inline void Deindent() {
    indent_ -= 1;
  }

  inline auto GetIndent() const -> std::string {
    return std::string(indent_ + 1, ' ');
  }

 public:
  ModulePrinter(const char* file, const int line, const google::LogSeverity severity) :
    file_(file),
    line_(line),
    severity_(severity) {}
  ~ModulePrinter() = default;

  auto GetFile() const -> const char* {
    return file_.c_str();
  }

  auto GetLine() const -> int {
    return line_;
  }

  auto GetSeverity() const -> google::LogSeverity {
    return severity_;
  }

  void Print(Module* module);
  auto VisitLocal(LocalVariable* local) -> bool override;

 public:
  static inline void PrintModule(Module* module, const char* file, const int line,
                                 const google::LogSeverity severity = google::INFO) {
    ASSERT(module);
    ASSERT(file);
    ModulePrinter printer(file, line, severity);
    return printer.Print(module);
  }
};

#define PRINT_MODULE(Severity, Module) ModulePrinter::PrintModule((Module), __FILE__, __LINE__, google::Severity)

}  // namespace scm

#endif  // SCM_MODULE_H
