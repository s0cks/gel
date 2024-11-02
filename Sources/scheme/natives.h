#ifndef SCM_NATIVES_H
#define SCM_NATIVES_H

#include "scheme/native_procedure.h"

namespace scm::proc {
DECLARE_NATIVE_PROCEDURE(print);
DECLARE_NATIVE_PROCEDURE(import);
DECLARE_NATIVE_PROCEDURE(exit);
DECLARE_NATIVE_PROCEDURE(format);
DECLARE_NATIVE_PROCEDURE(list);
DECLARE_NATIVE_PROCEDURE(foreach);
DECLARE_NATIVE_PROCEDURE(map);
_DECLARE_NATIVE_PROCEDURE(type, "type?");

#ifdef SCM_DEBUG
_DECLARE_NATIVE_PROCEDURE(list_symbols, "list-symbols!");
#endif  // SCM_DEBUG
}  // namespace scm::proc

#endif  // SCM_NATIVES_H
