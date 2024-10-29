#ifndef SCM_FLAGS_H
#define SCM_FLAGS_H

#include <fmt/format.h>
#include <gflags/gflags.h>

#include <filesystem>
#include <optional>
#include <string>

namespace scm {
DECLARE_bool(eval);
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

static inline auto GetReportFilename(const std::string& filename) -> std::string {
  const auto reports_dir_flag = GetReportsDirFlag();
  const std::filesystem::path reports_dir =
      reports_dir_flag ? std::filesystem::path(*reports_dir_flag) : std::filesystem::current_path();
  return fmt::format("{0:s}/{1:s}", reports_dir.string(), filename);
}
}  // namespace scm

#endif  // SCM_FLAGS_H
