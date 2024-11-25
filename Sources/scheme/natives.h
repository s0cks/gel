#ifndef SCM_NATIVES_H
#define SCM_NATIVES_H

#include "scheme/native_procedure.h"

namespace scm::proc {
DECLARE_NATIVE_PROCEDURE(print);
DECLARE_NATIVE_PROCEDURE(import);
DECLARE_NATIVE_PROCEDURE(exit);
DECLARE_NATIVE_PROCEDURE(format);
DECLARE_NATIVE_PROCEDURE(list);
DECLARE_NATIVE_PROCEDURE(random);
_DECLARE_NATIVE_PROCEDURE(type, "type?");
_DECLARE_NATIVE_PROCEDURE(rand_range, "random:range");
_DECLARE_NATIVE_PROCEDURE(set_car, "set-car!");
_DECLARE_NATIVE_PROCEDURE(set_cdr, "set-cdr!");

#ifdef SCM_DEBUG
_DECLARE_NATIVE_PROCEDURE(minor_gc, "minor-gc!");
_DECLARE_NATIVE_PROCEDURE(frame, "frame?");
_DECLARE_NATIVE_PROCEDURE(list_symbols, "list-symbols!");
_DECLARE_NATIVE_PROCEDURE(list_classes, "list-classes!");
#endif  // SCM_DEBUG
}  // namespace scm::proc

#endif  // SCM_NATIVES_H
