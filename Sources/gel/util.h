#ifndef GEL_UTIL_H
#define GEL_UTIL_H

#include <glog/logging.h>

#include "gel/common.h"

namespace gel {
class PrettyLogger {
  using Severity = google::LogSeverity;
  DEFINE_NON_COPYABLE_TYPE(PrettyLogger);

 private:
  std::string file_;
  int line_;
  Severity severity_;
  int indent_;

 protected:
  explicit PrettyLogger(const char* file, const int line, const Severity severity, const int indent = 0) :
    file_(file),
    line_(line),
    severity_(severity),
    indent_(indent) {}

  auto indent() const -> int {
    return indent_;
  }

  void Indent() {
    indent_ += 1;
  }

  void Deindent() {
    indent_ -= 1;
    ASSERT(indent_ >= 0);
  }

  auto GetIndentString() const -> std::string {
    return std::string((indent_ * 2), ' ');  // NOLINT(modernize-return-braced-init-list)
  }

  auto file() const -> const char* {
    return file_.data();
  }

  auto line() const -> int {
    return line_;
  }

 public:
  virtual ~PrettyLogger() = default;

  auto GetSeverity() const -> Severity {
    return severity_;
  }
};
}  // namespace gel

#endif  // GEL_UTIL_H
