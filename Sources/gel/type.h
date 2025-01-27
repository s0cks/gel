#ifndef GEL_TYPE_H
#define GEL_TYPE_H

#include "gel/rx.h"

namespace gel {
namespace expr {
class Expression;
}

namespace ir {
class Instruction;
class Definition;
}  // namespace ir

#define FOR_EACH_PRIMITIVE_TYPE(V) \
  V(Seq)                           \
  V(Class)                         \
  V(Field)                         \
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
  V(Error)                         \
  V(Namespace)                     \
  V(Set)                           \
  V(Map)                           \
  V(Module)                        \
  V(EventLoop)                     \
  V(Timer)                         \
  V(Buffer)

#define FOR_EACH_TYPE(V)     \
  FOR_EACH_PRIMITIVE_TYPE(V) \
  FOR_EACH_RX_TYPE(V)

class Object;
#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

using ObjectList = std::vector<Object*>;
}  // namespace gel

#endif  // GEL_TYPE_H
