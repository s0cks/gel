#ifndef GEL_ENTRY_H
#define GEL_ENTRY_H

#include <string>
#include "gel/common.h"

namespace gel {
#define FOR_EACH_ENTRY_TYPE(V) \
  V(NativeEntry) \
  V(RuntimeEntry)

class Entry;
#define FORWARD_DECLARE(Name) class Name;
  FOR_EACH_ENTRY_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class Entry {
  DEFINE_DEFAULT_COPYABLE_TYPE(Entry);
protected:
  Entry() = default;
public:
  virtual ~Entry() = default;
  virtual auto ToString() const -> std::string = 0;
  
#define DEFINE_TYPE_CHECK(Name)                                       \
  virtual auto As##Name() -> Name* { return nullptr; }                \
  inline auto Is##Name() -> bool { return As##Name() != nullptr; }
  FOR_EACH_ENTRY_TYPE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
};
}

#endif // GEL_ENTRY_H
