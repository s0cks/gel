#ifndef SCM_NATIVES_H
#define SCM_NATIVES_H

#include "scheme/common.h"
#include "scheme/procedure.h"

namespace scm::proc {
DECLARE_NATIVE_PROCEDURE(print);
DECLARE_NATIVE_PROCEDURE(import);
DECLARE_NATIVE_PROCEDURE(exit);
DECLARE_NATIVE_PROCEDURE(format);
_DECLARE_NATIVE_PROCEDURE(type, "type?");
_DECLARE_NATIVE_PROCEDURE(throw_exc, "throw!");
}  // namespace scm::proc

#endif  // SCM_NATIVES_H
