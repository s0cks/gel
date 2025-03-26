#ifndef GEL_SET_H
#define GEL_SET_H

#include "gel/native_procedure.h"
#include "gel/natives.h"
#include "gel/object.h"

namespace gel {
class Set : public Object {
 public:
  using StorageType = std::unordered_set<Object*, ObjectHasher, ObjectEquals>;

 private:
  StorageType data_;

 protected:
  Set(const StorageType& data) :
    Object(),
    data_(data) {}

  inline auto Find(Object* rhs) const -> StorageType::const_iterator {
    return data().find(rhs);
  }

 public:
  ~Set() override = default;

  auto data() const -> const StorageType& {
    return data_;
  }

  auto begin() const -> StorageType::const_iterator {
    return std::begin(data());
  }

  auto end() const -> StorageType::const_iterator {
    return std::end(data());
  }

  auto GetSize() const -> uword {
    return data().size();
  }

  inline auto IsEmpty() const -> bool {
    return data().empty();
  }

  auto Contains(Object* rhs) const -> bool {
    const auto& pos = Find(rhs);
    return pos != std::end(data());
  }

  auto Insert(Object* rhs) -> bool {
    ASSERT(rhs);
    const auto [pos, success] = data_.insert(rhs);
    return success;
  }

  DECLARE_TYPE(Set);

 public:
  static void Init();
  static auto Of(Object* value) -> Set*;
  static inline auto Of(const StorageType& data = {}) -> Set* {
    return new Set(data);
  }

  static inline auto Of(const std::vector<Object*>& values) -> Set* {
    StorageType data{};
    data.insert(std::begin(values), std::end(values));
    return Of(data);
  }

  static inline auto Empty() -> Set* {
    return Of();
  }

  static auto Union(Set* lhs, Set* rhs) -> Set*;
  static auto Difference(Set* lhs, Set* rhs) -> Set*;
  static auto Intersection(Set* lhs, Set* rhs) -> Set*;
};

namespace proc {
DECLARE_GEL_NATIVE_PROCEDURE(union);
DECLARE_GEL_NATIVE_PROCEDURE(difference);
DECLARE_GEL_NATIVE_PROCEDURE(intersection);
// _DECLARE_GEL_NATIVE_PROCEDURE(superset, "superset?");
_DECLARE_GEL_NATIVE_PROCEDURE(subset, "subset?");

#define _DECLARE_SET_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(set_##Name, "Set:" Sym)
#define DECLARE_SET_PROCEDURE(Name)       _DECLARE_SET_PROCEDURE(Name, #Name);

DECLARE_SET_PROCEDURE(insert);
_DECLARE_SET_PROCEDURE(contains, "contains?");
DECLARE_SET_PROCEDURE(count);
_DECLARE_SET_PROCEDURE(empty, "empty?");

#undef _DECLARE_SET_PROCEDURE
#undef DECLARE_SET_PROCEDURE
}  // namespace proc
}  // namespace gel

#endif  // GEL_SET_H
