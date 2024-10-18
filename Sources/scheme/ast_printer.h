#ifndef SCM_AST_PRINTER_H
#define SCM_AST_PRINTER_H

#include <glog/logging.h>

#include "scheme/ast.h"
#include "scheme/common.h"

namespace scm::ast {
class AstPrinter : public NodeVisitor {
  DEFINE_NON_COPYABLE_TYPE(AstPrinter);

  using Severity = google::LogSeverity;

 private:
  const char* file_;
  int line_;
  Severity severity_;
  int indent_;

  AstPrinter(const char* file, const int line, const Severity severity, const int indent) :
    file_(file),
    line_(line),
    severity_(severity),
    indent_(indent) {}

  auto GetFile() const -> const char* {
    return file_;
  }

  auto GetLine() const -> int {
    return line_;
  }

  void Indent() {
    indent_ += 1;
  }

  void Deindent() {
    indent_ -= 1;
  }

  auto GetIndentString() const -> std::string {
    return std::string(GetIndent(), ' ');  // NOLINT(modernize-return-braced-init-list)
  }

 public:
  ~AstPrinter() override = default;

  auto GetSeverity() const -> Severity {
    return severity_;
  }

  auto GetIndent() const -> int {
    return indent_;
  }

#define DECLARE_VISIT(Name) virtual auto Visit##Name(ast::Name* name) -> bool override;
  FOR_EACH_AST_NODE(DECLARE_VISIT)
#undef DECLARE_VISIT

 public:
  static inline auto Print(Node* node, const char* file, const int line, const Severity severity, const int indent = 0) -> bool {
    AstPrinter printer(file, line, severity, indent);
    return node->Accept(&printer);
  }
};
}  // namespace scm::ast

#endif  // SCM_AST_PRINTER_H
