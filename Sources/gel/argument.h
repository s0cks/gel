#ifndef GEL_ARGUMENT_H
#define GEL_ARGUMENT_H

#include <set>

#include "gel/object.h"

namespace gel {

class Argument {
  DEFINE_DEFAULT_COPYABLE_TYPE(Argument);

 private:
  uint64_t index_;
  std::string name_;
  bool optional_;
  bool vararg_;

 public:
  Argument(const uint64_t index, const std::string& name, const bool optional, const bool vararg) :
    index_(index),
    name_(name),
    optional_(optional),
    vararg_(vararg) {}
  ~Argument() = default;

  auto GetIndex() const -> uint64_t {
    return index_;
  }

  auto GetName() const -> const std::string& {
    return name_;
  }

  auto IsOptional() const -> bool {
    return optional_;
  }

  auto IsVararg() const -> bool {
    return vararg_;
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
    stream << "name=" << rhs.GetName() << ", ";
    stream << "optional=" << rhs.IsOptional() << ", ";
    stream << "vararg=" << rhs.IsVararg();
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
    stream << arg.GetName();
    if (--remaining > 0)
      stream << ", ";
  }
  stream << "]";
  return stream;
}
}  // namespace gel

#endif  // GEL_ARGUMENT_H
