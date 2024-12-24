#ifndef GEL_NATIVES_H
#define GEL_NATIVES_H

#include "gel/native_procedure.h"

namespace gel::proc {
_DECLARE_NATIVE_PROCEDURE(gel_docs, "gel/docs?");
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
_DECLARE_NATIVE_PROCEDURE(array_new, "Array/new");
_DECLARE_NATIVE_PROCEDURE(array_get, "Array/get");
_DECLARE_NATIVE_PROCEDURE(array_set, "Array/set!");
_DECLARE_NATIVE_PROCEDURE(array_length, "Array/count");  // TODO: rename
DECLARE_NATIVE_PROCEDURE(hashcode);
_DECLARE_NATIVE_PROCEDURE(gel_sizeof, "sizeof");

// Set
// ----------------------------------------------------------------------------------------------------
#define _DECLARE_SET_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(set_##Name, "Set/" Sym)
#define DECLARE_SET_PROCEDURE(Name)       _DECLARE_SET_PROCEDURE(Name, #Name);
// procedures
// ----------------------------------------------------------------------------------------------------
DECLARE_SET_PROCEDURE(contains);
DECLARE_SET_PROCEDURE(count);
_DECLARE_SET_PROCEDURE(empty, "empty?");
// ----------------------------------------------------------------------------------------------------
#undef _DECLARE_SET_PROCEDURE
#undef DECLARE_SET_PROCEDURE
// ----------------------------------------------------------------------------------------------------

#ifdef GEL_ENABLE_RX
#define _DECLARE_NATIVE_RX_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(rx_##Name, "rx/" Sym)
#define DECLARE_NATIVE_RX_PROCEDURE(Name)       _DECLARE_NATIVE_RX_PROCEDURE(Name, #Name)

DECLARE_NATIVE_RX_PROCEDURE(observer);
DECLARE_NATIVE_RX_PROCEDURE(first);
DECLARE_NATIVE_RX_PROCEDURE(last);
DECLARE_NATIVE_RX_PROCEDURE(skip);
DECLARE_NATIVE_RX_PROCEDURE(take);
DECLARE_NATIVE_RX_PROCEDURE(take_last);
DECLARE_NATIVE_RX_PROCEDURE(filter);
// TODO: DECLARE_NATIVE_RX_PROCEDURE(reduce);
// TODO: _DECLARE_NATIVE_RX_PROCEDURE(group_by, "group-by");
// TODO: _DECLARE_NATIVE_RX_PROCEDURE(take_until, "take-until");
DECLARE_NATIVE_RX_PROCEDURE(buffer);
DECLARE_NATIVE_RX_PROCEDURE(observable);
DECLARE_NATIVE_RX_PROCEDURE(subscribe);
DECLARE_NATIVE_RX_PROCEDURE(map);
_DECLARE_NATIVE_RX_PROCEDURE(take_while, "take-while");
_DECLARE_NATIVE_RX_PROCEDURE(publish_subject, "publish-subject");
_DECLARE_NATIVE_RX_PROCEDURE(replay_subject, "replay-subject");
DECLARE_NATIVE_RX_PROCEDURE(publish);
DECLARE_NATIVE_RX_PROCEDURE(complete);
_DECLARE_NATIVE_RX_PROCEDURE(publish_error, "publish-error");

#ifdef GEL_DEBUG
_DECLARE_NATIVE_RX_PROCEDURE(get_operators, "get-operators");
#endif  // GEL_DEBUG

#undef _DECLARE_NATIVE_RX_PROCEDURE
#undef DECLARE_NATIVE_RX_PROCEDURE

#endif  // GEL_ENABLE_RX

#ifdef GEL_DEBUG
_DECLARE_NATIVE_PROCEDURE(gel_get_roots, "gel/get-roots");
_DECLARE_NATIVE_PROCEDURE(gel_minor_gc, "gel/minor-gc!");
_DECLARE_NATIVE_PROCEDURE(gel_major_gc, "gel/major-gc!");
_DECLARE_NATIVE_PROCEDURE(gel_print_heap, "gel/print-heap");
_DECLARE_NATIVE_PROCEDURE(gel_print_new_zone, "gel/print-new-zone");
_DECLARE_NATIVE_PROCEDURE(gel_print_old_zone, "gel/print-old-zone");
_DECLARE_NATIVE_PROCEDURE(gel_get_debug, "gel/debug?");
_DECLARE_NATIVE_PROCEDURE(gel_get_frame, "gel/get-frame");
_DECLARE_NATIVE_PROCEDURE(gel_print_st, "gel/print-st");
_DECLARE_NATIVE_PROCEDURE(gel_get_locals, "gel/get-locals");
_DECLARE_NATIVE_PROCEDURE(gel_get_classes, "gel/get-classes");
_DECLARE_NATIVE_PROCEDURE(gel_get_target_triple, "gel/get-target-triple");
_DECLARE_NATIVE_PROCEDURE(gel_get_natives, "gel/get-natives");
_DECLARE_NATIVE_PROCEDURE(gel_get_compile_time, "gel/compile-time?");
#endif  // GEL_DEBUG

}  // namespace gel::proc

#endif  // GEL_NATIVES_H
