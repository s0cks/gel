#ifndef GEL_TYPE_H
#define GEL_TYPE_H

#include "gel/rx.h"

namespace gel {
namespace expr {
class Expression;
}

namespace instr {
class Instruction;
class Definition;
}  // namespace instr

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
}  // namespace gel

#endif  // GEL_TYPE_H