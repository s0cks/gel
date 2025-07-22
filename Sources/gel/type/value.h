#ifndef GEL_VALUE_H
#define GEL_VALUE_H

#include <compare>

#include "gel/allocator.h"
#include "gel/common.h"
#include "gel/hashcode.h"
#include "gel/type/type.h"

namespace gel {
class Value : public HeapObject {
  DEFINE_NON_COPYABLE_TYPE(Value);

 protected:
  Value() = default;

 public:
  virtual ~Value() = default;
  virtual auto GetHashCode() const -> HashCode = 0;
  virtual auto Equals(Value* rhs) const -> bool = 0;
  virtual auto Compare(Value* rhs) const -> std::strong_ordering = 0;

#define DEFINE_TYPE_CHECK(Name)      \
  virtual auto As##Name() -> Name* { \
    return nullptr;                  \
  }                                  \
  inline auto Is##Name() -> bool {   \
    return As##Name() != nullptr;    \
  }
  FOR_EACH_TYPE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
};

#define DECLARE_VALUE_TYPE(Name)                                   \
  DEFINE_NON_COPYABLE_TYPE(Name);                                  \
                                                                   \
 public:                                                           \
  auto As##Name() -> Name* override {                              \
    return this;                                                   \
  }                                                                \
  auto GetHashCode() const -> HashCode override;                   \
  auto ToString() const -> std::string override;                   \
  auto Equals(Value* rhs) const -> bool override;                  \
  auto Compare(Value* rhs) const -> std::strong_ordering override; \
  static auto operator new(const size_t sz)->void*;                \
  static inline void operator delete(void* ptr) {                  \
    ASSERT(ptr);                                                   \
  }
}  // namespace gel

#endif  // GEL_VALUE_H
