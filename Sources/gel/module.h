#ifndef GEL_MODULE_H
#define GEL_MODULE_H

#include "gel/object.h"

namespace gel {
class Module : public Object {
 private:
  String* name_;

 protected:
  explicit Module(String* name) :
    Object(),
    name_(name) {
    ASSERT(name_);
  }

 public:
  ~Module() override = default;

  auto GetName() const -> String* {
    return name_;
  }

  DECLARE_TYPE(Module);

 public:
  static inline auto New(String* name) -> Module* {
    ASSERT(name);
    return new Module(name);
  }
};
}  // namespace gel

#endif  // GEL_MODULE_H
