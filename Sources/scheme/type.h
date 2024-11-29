#ifndef SCM_TYPE_H
#define SCM_TYPE_H

#include "scheme/rx.h"

namespace scm {
#define FOR_EACH_PRIMITIVE_TYPE(V) \
  V(Class)                         \
  V(Bool)                          \
  V(Number)                        \
  V(Double)                        \
  V(Long)                          \
  V(String)                        \
  V(Symbol)                        \
  V(Macro)                         \
  V(Procedure)                     \
  V(Lambda)                        \
  V(NativeProcedure)               \
  V(Pair)                          \
  V(Script)                        \
  V(Error)

#define FOR_EACH_TYPE(V)     \
  FOR_EACH_PRIMITIVE_TYPE(V) \
  FOR_EACH_RX_TYPE(V)

class Object;
#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

using ObjectList = std::vector<Object*>;
}  // namespace scm

#endif  // SCM_TYPE_H
