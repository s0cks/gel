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
    if (remaining != 1 && --remaining >= 1)
      stream << ", ";
  }
  stream << "]";
  return stream;
}
}  // namespace gel

#endif  // GEL_ARGUMENT_H
