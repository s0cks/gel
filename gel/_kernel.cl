(ns _kernel
  "The main namespace for gel."
  (defnative gel:docs? [o]
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
  ;; TODO: reduce visibility to debug builds only:
  ;; --------------------------------------------------
  (defnative gel:minor-gc! []
    "Performs a minor garbage collection cycle.")
  (defnative gel:major-gc! []
    "Performs a major garbage collection cycle.")
  (defnative gel:debug? []
    "Returns whether or not this is a debug instance of gelrt.")
  (defnative gel:get-frame []
    "Returns the current StackFrame from gelrt.")
  (defnative gel:get-locals []
    "Returns the current LocalScope from gelrt.")
  (defnative gel:get-classes []
    "Returns a list of the current register Classes in gelrt.")
  (defnative gel:get-target-triple []
    "Returns the current target triple for gelrt.")
  (defnative gel:get-natives []
    "Returns a list of native functions register in gelrt.")
  (defnative gel:compile-time? [f]
    "Returns the compilation time of a function [f] in nanoseconds.")
  (defnative gel:get-roots []
    "Returns the roots for the GC.")
  (defn gel:inspect [o]
    (when (#Procedure? o)
      (print (format "compiled in {}ns." (gel:compile-time? o)))))
  ;; --------------------------------------------------
  ;; pair accessors
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
  ;;TODO:
  ;; - (def PI 3.14159)
  ;; - (def TAU (* 2 PI))
  (defn sq [x]
    "Returns x * x."
    (* x x))
  (defn zero? [x]
    "Returns true if x is 0."
    (eq? x 0))
  (defn even? [x]
    "Returns true if x is even."
    (zero? (% x 2)))
  (defn max [a b]
    "Returns the max value between a & b."
    (cond (> a b) a
      b))
  (defn min [a b]
    "Returns the min value between a"
    (cond (< a b) a
      b))
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