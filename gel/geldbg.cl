(ns gel
  (defnative print-args [func]
  "Pretty prints the arguments of function [func].")
  (defnative minor-gc! []
    "Performs a minor garbage collection cycle.")
  (defnative major-gc! []
    "Performs a major garbage collection cycle.")
  (defnative get-frame []
    "Returns the current StackFrame from gelrt.")
  (defnative print-st []
    "Prints the current StackTrace for the gelrt.")
  (defnative get-locals []
    "Returns the current LocalScope from gelrt.")
  (defnative get-target-triple []
    "Returns the current target triple for gelrt.")
  (defnative get-natives []
    "Returns a list of native functions register in gelrt.")
  (defnative compile-time? [f]
    "Returns the compilation time of a function [f] in nanoseconds.")
  (defnative print-roots []
    "Prints the roots for the GC.")
  (defn inspect [o]
    (when (#Procedure? o)
      (print (format "compiled in {}ns." (gel:compile-time? o)))))
  (defnative print-heap []
    "Prints the heap information to the terminal.")
  (defnative print-new-zone []
    "Prints the heap's new zone information to the terminal.")
  (defnative print-old-zone []
    "Prints the heap's old zone information to the terminal."))