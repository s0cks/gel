#include <cmath>
#include <cstdlib>
#include <glog/logging.h>

#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/plugin.h"

using namespace gel;

#define _DECLARE_MATH_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(math_##Name, "math/" #Sym)
#define DECLARE_MATH_PROCEDURE(Name)       _DECLARE_MATH_PROCEDURE(Name, Name)

DECLARE_MATH_PROCEDURE(acos);
DECLARE_MATH_PROCEDURE(asin);
DECLARE_MATH_PROCEDURE(atan);
DECLARE_MATH_PROCEDURE(ceil);
DECLARE_MATH_PROCEDURE(cos);
DECLARE_MATH_PROCEDURE(cosh);
DECLARE_MATH_PROCEDURE(floor);
DECLARE_MATH_PROCEDURE(log);
DECLARE_MATH_PROCEDURE(log10);
DECLARE_MATH_PROCEDURE(pow);
DECLARE_MATH_PROCEDURE(round);
DECLARE_MATH_PROCEDURE(sin);
DECLARE_MATH_PROCEDURE(sinh);
DECLARE_MATH_PROCEDURE(sqrt);
DECLARE_MATH_PROCEDURE(tan);
DECLARE_MATH_PROCEDURE(tanh);
_DECLARE_MATH_PROCEDURE(to_radians, "to-radians");
_DECLARE_MATH_PROCEDURE(to_degrees, "to-degrees");

#undef _DECLARE_MATH_PROCEDURE
#undef DECLARE_MATH_PROCEDURE

#define MATH_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(math_##Name)

#define MATH_PROCEDURE_F1(Name)              \
  MATH_PROCEDURE_F(Name) {                   \
    NativeArgument<0, Double> value(args);   \
    CHECK_NATIVE_ARG(value);                 \
    return ReturnDouble(Name(value->Get())); \
  }

#define MATH_PROCEDURE_F2(Name)                    \
  MATH_PROCEDURE_F(Name) {                         \
    NativeArgument<0, Double> a(args);             \
    CHECK_NATIVE_ARG(a);                           \
    NativeArgument<0, Double> b(args);             \
    CHECK_NATIVE_ARG(b);                           \
    return ReturnDouble(Name(a->Get(), b->Get())); \
  }

MATH_PROCEDURE_F1(acos);
MATH_PROCEDURE_F1(asin);
MATH_PROCEDURE_F1(atan);
MATH_PROCEDURE_F1(ceil);
MATH_PROCEDURE_F1(cos);
MATH_PROCEDURE_F1(cosh);
MATH_PROCEDURE_F1(floor);
MATH_PROCEDURE_F1(log);
MATH_PROCEDURE_F1(log10);
MATH_PROCEDURE_F2(pow);
MATH_PROCEDURE_F1(round);
MATH_PROCEDURE_F1(sin);
MATH_PROCEDURE_F1(sinh);
MATH_PROCEDURE_F1(sqrt);
MATH_PROCEDURE_F1(tan);
MATH_PROCEDURE_F1(tanh);

#undef MATH_PROCEDURE_F2
#undef MATH_PROCEDURE_F1
#undef MATH_PROCEDURE_F

#define INIT_MATH_NATIVE(Name) InitNative<math_##Name>()

DEFINE_PLUGIN(Math) {
  INIT_MATH_NATIVE(acos);
  INIT_MATH_NATIVE(asin);
  INIT_MATH_NATIVE(atan);
  INIT_MATH_NATIVE(ceil);
  INIT_MATH_NATIVE(cos);
  INIT_MATH_NATIVE(cosh);
  INIT_MATH_NATIVE(floor);
  INIT_MATH_NATIVE(log);
  INIT_MATH_NATIVE(log10);
  INIT_MATH_NATIVE(pow);
  INIT_MATH_NATIVE(round);
  INIT_MATH_NATIVE(sin);
  INIT_MATH_NATIVE(sinh);
  INIT_MATH_NATIVE(sqrt);
  INIT_MATH_NATIVE(tan);
  INIT_MATH_NATIVE(tanh);
  return EXIT_SUCCESS;
}