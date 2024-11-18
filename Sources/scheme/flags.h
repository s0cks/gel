#ifndef SCM_FLAGS_H
#define SCM_FLAGS_H

#include <fmt/format.h>
#include <gflags/gflags.h>

#include <filesystem>
#include <optional>
#include <string>

#include "scheme/platform.h"

namespace scm {
#if defined(ARCH_IS_X64) || defined(ARCH_IS_ARM64)

#define DECLARE_word(Name) DECLARE_int64(Name);
#define DEFINE_word(Name, Default, Description) DEFINE_int64(Name, Default, Description);
#define DECLARE_uword(Name) DECLARE_uint64(Name);
#define DEFINE_uword(Name, Default, Description) DEFINE_uint64(Name, Default, Description);

#elif defined(ARCH_IS_X32)

#define DECLARE_word(Name) DECLARE_int32(Name);
#define DEFINE_word(Name, Default, Description) DEFINE_int32(Name, Default, Description);
#define DECLARE_uword(Name) DECLARE_uint32(Name);
#define DEFINE_uword(Name, Default, Description) DEFINE_uint32(Name, Default, Description);

#else
#error "Unsupported Operating System"
#endif

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
