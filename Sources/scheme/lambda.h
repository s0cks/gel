#ifndef SCM_LAMBDA_H
#define SCM_LAMBDA_H

#include <set>

#include "scheme/common.h"
#include "scheme/procedure.h"

namespace scm {
namespace expr {
class Expression;
}

class Argument {
  DEFINE_DEFAULT_COPYABLE_TYPE(Argument);

 private:
  uint64_t index_;
  std::string name_;

 public:
  Argument(const uint64_t index, const std::string& name) :
    index_(index),
    name_(name) {}
  ~Argument() = default;

  auto GetIndex() const -> uint64_t {
    return index_;
  }

  auto GetName() const -> const std::string& {
    return name_;
  }

  auto operator==(const Argument& rhs) const -> bool {
    return GetIndex() == rhs.GetIndex() && GetName() == rhs.GetName();
  }

  auto operator!=(const Argument& rhs) const -> bool {
    return GetIndex() != rhs.GetIndex() || GetName() != rhs.GetName();
  }

  auto operator<(const Argument& rhs) const -> bool {
    return GetIndex() < rhs.GetIndex();
  }

  friend auto operator<<(std::ostream& stream, const Argument& rhs) -> std::ostream& {
    stream << "Argument(";
    stream << "index=" << rhs.GetIndex() << ", ";
    stream << "name=" << rhs.GetName();
    stream << ")";
    return stream;
  }

 public:
  struct Comparator {
    auto operator()(const Argument& lhs, const Argument& rhs) const -> bool {
      return lhs.GetIndex() < rhs.GetIndex();
    }
  };
};

using ArgumentSet = std::set<Argument, Argument::Comparator>;

static inline auto operator<<(std::ostream& stream, const ArgumentSet& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& arg : rhs) {
    stream << arg;
    if (--remaining >= 1)
      stream << ", ";
  }
  stream << "]";
  return stream;
}

class Lambda : public Procedure {
 private:
  ArgumentSet args_;
  expr::Expression* body_;

 protected:
  explicit Lambda(const ArgumentSet& args, expr::Expression* body) :
    Procedure(),
    args_(args),
    body_(body) {}

 public:
  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetBody() const -> expr::Expression* {
    return body_;
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return args_.size();
  }

  auto Apply(Runtime* state) const -> bool override;
  DECLARE_TYPE(Lambda);

 public:
  static inline auto New(const ArgumentSet& args, expr::Expression* body) -> Lambda* {
    return new Lambda(args, body);
  }
};
}  // namespace scm

#endif  // SCM_LAMBDA_H
