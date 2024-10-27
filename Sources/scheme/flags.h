#ifndef SCM_FLAGS_H
#define SCM_FLAGS_H

#include <gflags/gflags.h>

#include <optional>
#include <string>

namespace scm {
DECLARE_bool(dump_ast);
DECLARE_bool(dump_flow_graph);
DECLARE_string(reports_dir);
DECLARE_string(expr);
DECLARE_string(module);

static inline auto GetReportsDirFlag() -> std::optional<std::string> {
  if (FLAGS_reports_dir.empty())
    return std::nullopt;
  return {FLAGS_reports_dir};
}

static inline auto GetExpressionFlag() -> std::optional<std::string> {
  if (FLAGS_expr.empty())
    return std::nullopt;
  return {FLAGS_expr};
}

static inline auto GetModuleFlag() -> std::optional<std::string> {
  if (FLAGS_module.empty())
    return std::nullopt;
  return {FLAGS_module};
}
}  // namespace scm

#endif  // SCM_FLAGS_H
