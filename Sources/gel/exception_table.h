#ifndef GEL_EXCEPTION_TABLE_H
#define GEL_EXCEPTION_TABLE_H

#include "common.h"
#include "platform.h"

namespace gel {
class ExceptionTable {
  DEFINE_DEFAULT_COPYABLE_TYPE(ExceptionTable);

 public:
  struct Entry {
    uword start;
    uword finish;
    uword target;

    auto ToString() const -> std::string;

    friend auto operator<<(std::ostream& stream, const Entry& rhs) -> std::ostream& {
      return stream << rhs.ToString();
    }
  };

  using EntryList = std::vector<Entry>;

 private:
  EntryList entries_{};

 public:
  ExceptionTable() = default;
  ~ExceptionTable() = default;

  inline auto entries() const -> const EntryList& {
    return entries_;
  }

  inline auto begin() const -> EntryList::const_iterator {
    return std::begin(entries());
  }

  inline auto end() const -> EntryList::const_iterator {
    return std::end(entries());
  }

  void Add(const uword start, const uword finish, const uword target) {
    entries_.emplace_back(start, finish, target);
  }

  auto ToString() const -> std::string;
};

}  // namespace gel

#endif  // GEL_EXCEPTION_TABLE_H
