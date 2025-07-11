#ifndef GEL_MAP_H
#define GEL_MAP_H

#include "gel/common.h"
#include "gel/native_procedure.h"
#include "gel/object.h"

namespace gel {
class Map : public Object {
 public:
  using StorageType = std::unordered_map<Symbol*, Object*, ObjectHasher, ObjectComparator>;
  using Iter = StorageType::iterator;
  using ConstIter = StorageType::const_iterator;

 private:
  StorageType data_;

  explicit Map(const StorageType& data) :
    Object(),
    data_(data) {}

  inline auto Find(Symbol* rhs) const -> ConstIter {
    return data().find(rhs);
  }

 public:
  ~Map() override = default;

  inline auto data() const -> const StorageType& {
    return data_;
  }

  inline auto begin() -> StorageType::iterator {
    return std::begin(data_);
  }

  inline auto begin() const -> StorageType::const_iterator {
    return std::begin(data());
  }

  inline auto end() -> StorageType::iterator {
    return std::end(data_);
  }

  inline auto end() const -> StorageType::const_iterator {
    return std::end(data());
  }

  auto GetSize() const -> uword {
    return data_.size();
  }

  auto IsEmpty() const -> bool {
    return data_.empty();
  }

  auto Contains(Symbol* rhs) const -> bool {
    return Find(rhs) != std::end(data());
  }

  auto Get(Symbol* key) const -> Object*;
  DECLARE_TYPE(Map);

 public:
  static void Init();
  static inline auto New(const StorageType& data = {}) -> Map* {
    return new Map(data);
  }
};

namespace proc {
#define _DECLARE_MAP_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(map_##Name, "map:" Sym)
#define DECLARE_MAP_PROCEDURE(Name)       _DECLARE_MAP_PROCEDURE(Name, #Name);

DECLARE_MAP_PROCEDURE(contains);
DECLARE_MAP_PROCEDURE(size);
DECLARE_MAP_PROCEDURE(get);
_DECLARE_MAP_PROCEDURE(empty, "empty?");

#undef _DECLARE_MAP_PROCEDURE
#undef DECLARE_MAP_PROCEDURE
}  // namespace proc
}  // namespace gel

#endif  // GEL_MAP_H
