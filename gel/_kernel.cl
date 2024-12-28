;; ---------------------------------------------------------------------------------
;; Timers
;; ---------------------------------------------------------------------------------
(ns Timer
  (defnative start [idx]
    "Starts Timer [idx].")
  (defnative stop [idx]
    "Stops Timer [idx].")
  (defnative again [idx]
    "Runs Timer [idx] again.")
  (defnative get-due-in [idx]
    "Returns the number of milliseconds Timer [idx] is due in.")
  (defnative get-repeat [idx]
    "Returns the repeat value of Timer [idx].")
  (defnative set-repeat! [idx repeat]
    "Sets the repeat value of Timer [idx] to [repeat]."))
;; ---------------------------------------------------------------------------------
;; Maps
;; ---------------------------------------------------------------------------------
(ns Map
  (defnative get [m k]
    "Returns the value of key [k] in Map [m].")
  (defnative contains [m k]
    "Returns whether or not key [k] is in Map [m].")
  (defnative size [m]
    "Returns the number of items in Map [m].")
  (defnative empty? [m]
    "Returns whether or not Map [m] is empty."))
;; ---------------------------------------------------------------------------------
;; Sets
;; ---------------------------------------------------------------------------------
(ns Set
  (defnative contains [s o]
    "Returns whether or not Object [o] is in Set [s].")
  (defnative count [s]
    "Returns the number of items in Set [s].")
  (defnative empty? [s]
    "Returns whether or not Set [s] is empty."))
;; ---------------------------------------------------------------------------------
;; Arrays
;; ---------------------------------------------------------------------------------
(ns Array
  (defnative Array/get [a i]
    "Returns the value at index [i] for Array [a].")
  (defnative Array/set! [a i v]
    "Sets the value at index [i] in Array [a] to [v].")
  (defnative Array/count [a]
    "Returns the length of Array [a]."))
;; ---------------------------------------------------------------------------------

;; ---------------------------------------------------------------------------------
;; _kernel
;; ---------------------------------------------------------------------------------
(ns _kernel
  "The main namespace for gel."
  (defnative hashcode [v]
    "Returns the hashcode of value [v].")
  (defnative sizeof [o]
    "Returns the size of Object [o] in bytes.")
  (defnative gel/docs? [o]
    "Returns the docstring attached to the supplied Object [o].")
  (defnative format [pattern args...]
    "Returns a formatted String using the supplied [pattern] and [args...].")
  (defnative print [value]
    "Prints the supplied [value] to the console.")
  ;; TODO: remove (list ...)
  (defnative list [values...]
    "Returns a new list using the supplied [values...]")
  ;; Random
  (defnative random []
    "Returns a random Long.")
  (defnative random:range [min max]
    "Returns a random Long in the range of [min] to [max].")
  (defnative type? [o]
    "Returns the type of Object [o].")
  (defnative set-car! [p v]
    "Sets the first value of Pair [p] to [v].")
  (defnative set-cdr! [p v]
    "Sets the second value of Pair [p] to [v].")
  (defnative dlopen [p]
    "Opens the shared library from file at path [p].")
  (defnative create-timer [on_tick timeout repeat]
    "Starts a new Timer on the EventLoop.")
  (defmacro interval [on_tick repeat]
    (create-timer on_tick 0 repeat))
  (defmacro timeout [on_tick timeout]
    (create-timer on_tick timeout 0))

  ;; ---------------------------------------------------------------------------------
  ;; Event Loop
  ;; ---------------------------------------------------------------------------------
  (defnative get-event-loop []
    "Returns the EventLoop for the current thread.")
  ;; ---------------------------------------------------------------------------------

  ;; ---------------------------------------------------------------------------------
  ;; Buffers
  ;; ---------------------------------------------------------------------------------
  (defnative Buffer:get-capacity [b]
    "Returns the capacity of Buffer [b].")
  (defnative Buffer:get-length [b]
    "Returns the number of written bytes in Buffer [b].")
  ;; ---------------------------------------------------------------------------------


  ;; ---------------------------------------------------------------------------------
  ;; Debug Only - TODO: Reduce visibility
  ;; ---------------------------------------------------------------------------------
  (defnative gel/print-args [func]
    "Pretty prints the arguments of function [func].")
  (defnative gel/minor-gc! []
    "Performs a minor garbage collection cycle.")
  (defnative gel/major-gc! []
    "Performs a major garbage collection cycle.")
  (defnative gel/debug? []
    "Returns whether or not this is a debug instance of gelrt.")
  (defnative gel/get-frame []
    "Returns the current StackFrame from gelrt.")
  (defnative gel/print-st []
    "Prints the current StackTrace for the gelrt.")
  (defnative gel/get-locals []
    "Returns the current LocalScope from gelrt.")
  (defnative get-classes []
    "Returns a list of the current register Classes in gelrt.")
  (defnative get-class [s]
    "Returns the Class for Symbol [s].")
  (defnative get-namespace [s]
    "Returns the Namespace for Symbol [s].")
  (defnative ns:get [nsOrSym s]
    "Returns the value for Symbol [s] in Namespace [nsOrSym].")
  (defnative gel/get-target-triple []
    "Returns the current target triple for gelrt.")
  (defnative gel/get-natives []
    "Returns a list of native functions register in gelrt.")
  (defnative gel/compile-time? [f]
    "Returns the compilation time of a function [f] in nanoseconds.")
  (defnative gel/get-roots []
    "Returns the roots for the GC.")
  (defn gel/inspect [o]
    (when (#Procedure? o)
      (print (format "compiled in {}ns." (gel:compile-time? o)))))
  (defnative gel/print-heap []
    "Prints the heap information to the terminal.")
  (defnative gel/print-new-zone []
    "Prints the heap's new zone information to the terminal.")
  (defnative gel/print-old-zone []
    "Prints the heap's old zone information to the terminal.")
  (defmacro assert [test m]
    "Assert that Bool [test] is true, if not throw an Error w/ message [m]."
    (when (and (gel/debug?) (not test))
      (throw (Error m))))
  ;; ---------------------------------------------------------------------------------

  ;; ---------------------------------------------------------------------------------
  ;; Pair Accessors
  ;; ---------------------------------------------------------------------------------
  (defn caar [xs]
    (car (car xs)))
  (defn cadr [xs]
    "Returns the second item in a list."
    (car (cdr xs)))
  (defn cdar [xs]
    (cdr (car xs)))
  (defn cddr [xs]
    (cdr (cdr xs)))
  (defn caaar [xs]
    (car (car (car xs))))
  (defn caadr [xs]
    (car (car (cdr xs))))
  (defn cadar [xs]
    (car (cdr (car xs))))
  (defn caddr [xs]
    (car (cdr (cdr xs))))
  (defn cdaar [xs]
    (cdr (car (car xs))))
  (defn cdadr [xs]
    (cdr (car (cdr xs))))
  (defn cddar [xs]
    (cdr (cdr (car xs))))
  (defn cdddr [xs]
    (cdr (cdr (cdr xs))))
  (defn caaaar [xs]
    (car (car (car (car xs)))))
  (defn caaadr [xs]
    (car (car (car (cdr xs)))))
  (defn caadar [xs]
    (car (car (cdr (car xs)))))
  (defn caaddr [xs]
    (car (car (cdr (cdr xs)))))
  (defn cadaar [xs]
    (car (cdr (car (car xs)))))
  (defn cadadr [xs]
    (car (cdr (car (cdr xs)))))
  (defn caddar [xs]
    (car (cdr (cdr (car xs)))))
  (defn cadddr [xs]
    (car (cdr (cdr (cdr xs)))))
  (defn cdaaar [xs]
    (cdr (car (car (car xs)))))
  (defn cdaadr [xs]
    (cdr (car (car (cdr xs)))))
  (defn cdadar [xs]
    (cdr (car (cdr (car xs)))))
  (defn cdaddr [xs]
    (cdr (car (cdr (cdr xs)))))
  (defn cddaar [xs]
    (cdr (cdr (car (car xs)))))
  (defn cddadr [xs]
    (cdr (cdr (car (cdr xs)))))
  (defn cdddar [xs]
    (cdr (cdr (cdr (car xs)))))
  (defn cddddr [xs]
    (cdr (cdr (cdr (cdr xs)))))
  ;; ---------------------------------------------------------------------------------

  ;; ---------------------------------------------------------------------------------
  ;; Misc
  ;; ---------------------------------------------------------------------------------
  (defmacro newline []
    "Prints a platform specific newline to the console."
    (print ""))
  (defn inc [x]
    "Returns one more than [x]."
    (+ x 1))
  (defn dec [x]
    "Returns one less than [x]."
    (- x 1))
  ;;TODO:
  ;; - (def PI 3.14159)
  ;; - (def TAU (* 2 PI))
  (defmacro sq [x]
    "[x] * [x]"
    (* x x))
  (defmacro zero? [x]
    "Returns true if x is 0."
    (eq? x 0))
  (defmacro even? [x]
    "Returns true if x is even."
    (zero? (% x 2)))
  (defmacro odd? [x]
    "Returns true if x is even."
    (not (zero? (% x 2))))
  (defmacro false? [x]
    "Returns true if [x] is an instanceof false."
    (and (#Bool? x) (not x)))
  (defmacro true? [x]
    "Returns true if [x] is an instanceof true."
    (and (#Bool? x) x))
  (defn min [seq]
    "Returns the min value in Seq [seq]."
    ((fn [candidate tail]
      (cond (null? tail) candidate
        ($ ((fn [a b] (cond (< a b) a b)) candidate (car tail)) (cdr tail))))
      (car seq) (cdr seq)))
  (defn max [seq]
    "Returns the max value in Seq [seq]."
    ((fn [candidate tail]
      (cond (null? tail) candidate
        ($
          ((fn [a b]
            (cond (> a b) a b)) candidate
              (car tail))
          (cdr tail))))
      (car seq) (cdr seq)))
  (defn factorial [x]
    "Returns x!."
    (cond (eq? x 1) 1
      (* x (factorial (- x 1)))))
  (defn apply [f seq]
    (cond (null? seq) seq
      (begin
        (f (car seq))
        (apply f (cdr seq)))))
  (defn map [f seq]
    (cond (null? seq) seq
      (cons (f (car seq)) (map f (cdr seq)))))
  (defn filter [p seq]
    (cond (null? seq) seq
      (p (car seq)) (cons (car seq) (filter p (cdr seq)))
      (filter p (cdr seq))))
  (defn length [seq]
    "Returns the length of a sequence."
    (cond (null? seq) 0
      (+ 1 (length (cdr seq)))))
  (defn append [seq x]
    "Appends a sequence to another sequence."
    (cond (null? seq) x
      (cons (car seq) (append (cdr seq) x))))
  (defn nth [seq n]
    "Returns the nth value in a sequence."
    (when (or (> n (length seq)) (< n 0))
      (throw (format "Index `{}` out of bounds" n)))
    (cond (eq? n 0) (car seq)
      (nth (cdr seq) (- n 1)))))