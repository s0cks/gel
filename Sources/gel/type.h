#ifndef GEL_TYPE_H
#define GEL_TYPE_H

#include <vector>

#include "gel/rx.h"

namespace gel {
namespace expr {
class Expression;
}

namespace ir {
class Instruction;
class Definition;
}  // namespace ir

#ifdef GEL_ENABLE_GLM

#define FOR_EACH_GLM_TYPE(V) \
  V(Vec2)                    \
  V(Vec3)

#else

#define FOR_EACH_GLM_TYPE(V)

#endif  // GEL_ENABLE_GLM

#define FOR_EACH_PRIMITIVE_TYPE(V) \
  V(Seq)                           \
  V(Iterator)                      \
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
  V(Constructor)                   \
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
  V(Buffer)                        \
  V(EventEmitter)

#define FOR_EACH_TYPE(V)     \
  FOR_EACH_PRIMITIVE_TYPE(V) \
  FOR_EACH_RX_TYPE(V)        \
  FOR_EACH_GLM_TYPE(V)

class Object;
#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

template <typename T>
class Array;

using ObjectList = std::vector<Object*>;

#ifdef GEL_DEBUG
void PrintAllTypeSizes();
#endif  // GEL_DEBUG
}  // namespace gel

#endif  // GEL_TYPE_H
