#ifndef SCM_NATIVES_H
#define SCM_NATIVES_H

#include "scheme/native_procedure.h"

namespace scm::proc {
DECLARE_NATIVE_PROCEDURE(print);
DECLARE_NATIVE_PROCEDURE(import);
DECLARE_NATIVE_PROCEDURE(exit);
DECLARE_NATIVE_PROCEDURE(format);
DECLARE_NATIVE_PROCEDURE(list);
_DECLARE_NATIVE_PROCEDURE(type, "type?");
}  // namespace scm::proc

#endif  // SCM_NATIVES_H
