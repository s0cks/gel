;; ---------------------------------------------------------------------------------
;; Filesystem
;; ---------------------------------------------------------------------------------
(ns fs
  (defnative get-cwd []
    "Returns the current working directory (cwd).")
  (defnative stat [p on_next on_error on_finished]
    "Stats file at path [p] on the filesystem.")
  (defnative rename [old_path new_path on_error on_finished]
    "Renames a path at path [old_path] to path [new_path].")
  (defnative mkdir [p m on_success on_error on_finished]
    "Create a directory at the specified path [p] with mode [m]")
  (defnative rmdir [on_error on_finished])
  (defnative fsync [on_error on_finished])
  (defnative ftruncate [on_error on_finished])
  (defnative access [on_error on_finished])
  (defnative chmod [on_error on_finished])
  (defnative link [on_error on_finished])
  (defnative symlink [on_error on_finished])
  (defnative readlink [on_error on_finished])
  (defnative chown [on_error on_finished])
  (defnative copy-file [on_error on_finished]))