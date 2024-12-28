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
DECLARE_NATIVE_PROCEDURE(dlopen);
_DECLARE_NATIVE_PROCEDURE(get_event_loop, "get-event-loop");

// ----------------------------------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------------------------------
_DECLARE_NATIVE_PROCEDURE(get_class, "get-class");
_DECLARE_NATIVE_PROCEDURE(get_classes, "get-classes");
// ----------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------
// Namespaces
// ----------------------------------------------------------------------------------------------------
_DECLARE_NATIVE_PROCEDURE(get_namespace, "get-namespace");
_DECLARE_NATIVE_PROCEDURE(ns_get, "ns:get");
// ----------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------
// Timers
// ----------------------------------------------------------------------------------------------------
#define _DECLARE_TIMER_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(timer_##Name, "Timer/" Sym)
#define DECLARE_TIMER_PROCEDURE(Name)       _DECLARE_TIMER_PROCEDURE(Name, #Name);

DECLARE_TIMER_PROCEDURE(start);
DECLARE_TIMER_PROCEDURE(stop);
DECLARE_TIMER_PROCEDURE(again);
_DECLARE_TIMER_PROCEDURE(get_due_in, "get-due-in");
_DECLARE_TIMER_PROCEDURE(get_repeat, "get-repeat");
_DECLARE_TIMER_PROCEDURE(set_repeat, "set-repeat!");

_DECLARE_NATIVE_PROCEDURE(create_timer, "create-timer");
#undef _DECLARE_TIMER_PROCEDURE
#undef DECLARE_TIMER_PROCEDURE
// ----------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------
// Set
// ----------------------------------------------------------------------------------------------------
#define _DECLARE_SET_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(set_##Name, "Set/" Sym)
#define DECLARE_SET_PROCEDURE(Name)       _DECLARE_SET_PROCEDURE(Name, #Name);

DECLARE_SET_PROCEDURE(contains);
DECLARE_SET_PROCEDURE(count);
_DECLARE_SET_PROCEDURE(empty, "empty?");

#undef _DECLARE_SET_PROCEDURE
#undef DECLARE_SET_PROCEDURE
// ----------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------
// Map
// ----------------------------------------------------------------------------------------------------
#define _DECLARE_MAP_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(map_##Name, "Map/" Sym)
#define DECLARE_MAP_PROCEDURE(Name)       _DECLARE_MAP_PROCEDURE(Name, #Name);

DECLARE_MAP_PROCEDURE(contains);
DECLARE_MAP_PROCEDURE(size);
DECLARE_MAP_PROCEDURE(get);
_DECLARE_MAP_PROCEDURE(empty, "empty?");

#undef _DECLARE_SET_PROCEDURE
#undef DECLARE_SET_PROCEDURE
// ----------------------------------------------------------------------------------------------------

#ifdef GEL_ENABLE_RX
#define _DECLARE_NATIVE_RX_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(rx_##Name, "rx/" Sym)
#define DECLARE_NATIVE_RX_PROCEDURE(Name)       _DECLARE_NATIVE_RX_PROCEDURE(Name, #Name)

DECLARE_NATIVE_RX_PROCEDURE(observer);

// filter operators
DECLARE_NATIVE_RX_PROCEDURE(distinct);
_DECLARE_NATIVE_RX_PROCEDURE(distinct_until_changed, "distinct-until-changed");
_DECLARE_NATIVE_RX_PROCEDURE(element_at, "element-at");
DECLARE_NATIVE_RX_PROCEDURE(filter);
DECLARE_NATIVE_RX_PROCEDURE(first);
DECLARE_NATIVE_RX_PROCEDURE(last);
DECLARE_NATIVE_RX_PROCEDURE(skip);
DECLARE_NATIVE_RX_PROCEDURE(take);
DECLARE_NATIVE_RX_PROCEDURE(take_last);
// TODO:
// - (rx:throttle)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__filtering__operators.html#gada7d22ff6ee83d4aca44411561f56f98

// conditional operators
_DECLARE_NATIVE_RX_PROCEDURE(take_while, "take-while");
// TODO:
// - (rx:take-until)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__conditional__operators.html#gae40802b5608766bfcc1417e210f41707

// combing operators
DECLARE_NATIVE_RX_PROCEDURE(merge);
// TODO:
// - (rx:combine-latest)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__combining__operators.html#ga5c6073a2a2858a3f037a5ec930da33d2
// - (rx:merge-with)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__combining__operators.html#ga35392a84e52bf7101d0d0445ef391db7
// - (rx:start-with)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__combining__operators.html#gadf12c4de9702425819b0d34b5fdf7db3
// - (rx:with-latest-from)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__combining__operators.html#ga20acc2f23cb06163f9d5e56c5522ce6c
// - (rx:zip)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__combining__operators.html#ga83f723fc5e197ab21f67b879c5a668d7

// utility operators
// TODO:
// - (rx:debounce)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__utility__operators.html#ga49e91c6114f62ac261303814c818face
// - (rx:delay)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__utility__operators.html#gae38caa489cf43c0aca1aa1c8822aed60
// - (rx:repeat)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__utility__operators.html#ga41ef49dc75a3eadb0eb3fda7c3692c40
// - (rx:tap)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__utility__operators.html#gada6b3aa896ef87213836a70500d53be0
// - (rx:timeout)
// https://victimsnino.github.io/ReactivePlusPlus/v2/docs/html/group__utility__operators.html#ga08c63b0f0f0eae7e2ce0c4789569b9a7

// TODO: DECLARE_NATIVE_RX_PROCEDURE(reduce);
// TODO: _DECLARE_NATIVE_RX_PROCEDURE(group_by, "group-by");
// TODO: _DECLARE_NATIVE_RX_PROCEDURE(take_until, "take-until");
DECLARE_NATIVE_RX_PROCEDURE(buffer);
DECLARE_NATIVE_RX_PROCEDURE(observable);
DECLARE_NATIVE_RX_PROCEDURE(subscribe);
DECLARE_NATIVE_RX_PROCEDURE(map);
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

// TODO: switch based on sandbox flags
#define _DECLARE_FS_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(fs_##Name, "fs/" Sym);
#define DECLARE_FS_PROCEDURE(Name)       _DECLARE_FS_PROCEDURE(Name, #Name)

_DECLARE_FS_PROCEDURE(get_cwd, "get-cwd");
DECLARE_FS_PROCEDURE(stat);
DECLARE_FS_PROCEDURE(rename);
DECLARE_FS_PROCEDURE(mkdir);
DECLARE_FS_PROCEDURE(rmdir);
DECLARE_FS_PROCEDURE(fsync);
DECLARE_FS_PROCEDURE(ftruncate);
DECLARE_FS_PROCEDURE(access);
DECLARE_FS_PROCEDURE(chmod);
DECLARE_FS_PROCEDURE(link);
DECLARE_FS_PROCEDURE(symlink);
DECLARE_FS_PROCEDURE(readlink);
DECLARE_FS_PROCEDURE(chown);
_DECLARE_FS_PROCEDURE(copy_file, "copy-file");

#undef DECLARE_FS_PROCEDURE
#undef _DECLARE_FS_PROCEDURE

#ifdef GEL_DEBUG
_DECLARE_NATIVE_PROCEDURE(gel_print_args, "gel/print-args");
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
_DECLARE_NATIVE_PROCEDURE(gel_get_target_triple, "gel/get-target-triple");
_DECLARE_NATIVE_PROCEDURE(gel_get_natives, "gel/get-natives");
_DECLARE_NATIVE_PROCEDURE(gel_get_compile_time, "gel/compile-time?");
#endif  // GEL_DEBUG

}  // namespace gel::proc

#endif  // GEL_NATIVES_H
