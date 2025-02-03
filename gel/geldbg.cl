(ns gel
  (defnative gel/print-args [func]
  "Pretty prints the arguments of function [func].")
  (defnative gel/minor-gc! []
    "Performs a minor garbage collection cycle.")
  (defnative gel/major-gc! []
    "Performs a major garbage collection cycle.")
  (defnative gel/get-frame []
    "Returns the current StackFrame from gelrt.")
  (defnative gel/print-st []
    "Prints the current StackTrace for the gelrt.")
  (defnative gel/get-locals []
    "Returns the current LocalScope from gelrt.")
  (defnative gel/get-target-triple []
    "Returns the current target triple for gelrt.")
  (defnative gel/get-natives []
    "Returns a list of native functions register in gelrt.")
  (defnative gel/compile-time? [f]
    "Returns the compilation time of a function [f] in nanoseconds.")
  (defnative gel/print-roots []
    "Prints the roots for the GC.")
  (defn gel/inspect [o]
    (when (#Procedure? o)
      (print (format "compiled in {}ns." (gel:compile-time? o)))))
  (defnative gel/print-heap []
    "Prints the heap information to the terminal.")
  (defnative gel/print-new-zone []
    "Prints the heap's new zone information to the terminal.")
  (defnative gel/print-old-zone []
    "Prints the heap's old zone information to the terminal."))