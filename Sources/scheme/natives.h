#ifndef SCM_NATIVES_H
#define SCM_NATIVES_H

#include "scheme/common.h"
#include "scheme/procedure.h"

namespace scm::proc {
DECLARE_NATIVE_PROCEDURE(print);
_DECLARE_NATIVE_PROCEDURE(type, "type?");
}  // namespace scm::proc

#endif  // SCM_NATIVES_H
