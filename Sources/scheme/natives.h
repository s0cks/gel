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
_DECLARE_NATIVE_PROCEDURE(array_new, "array:new");
_DECLARE_NATIVE_PROCEDURE(array_get, "array:get");
_DECLARE_NATIVE_PROCEDURE(array_set, "array:set");
_DECLARE_NATIVE_PROCEDURE(array_length, "array:length");

#ifdef SCM_DEBUG
_DECLARE_NATIVE_PROCEDURE(scm_minor_gc, "scm:minor-gc!");
_DECLARE_NATIVE_PROCEDURE(scm_major_gc, "scm:major-gc!");
_DECLARE_NATIVE_PROCEDURE(scm_get_debug, "scm:debug?");
_DECLARE_NATIVE_PROCEDURE(scm_get_frame, "scm:get-frame");
_DECLARE_NATIVE_PROCEDURE(scm_get_locals, "scm:get-locals");
_DECLARE_NATIVE_PROCEDURE(scm_get_classes, "scm:get-classes");
_DECLARE_NATIVE_PROCEDURE(scm_get_target_triple, "scm:get-target-triple");
#endif  // SCM_DEBUG

}  // namespace scm::proc

#endif  // SCM_NATIVES_H
