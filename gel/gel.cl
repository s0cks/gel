(defmacro debug-only [exprs...]
  (when (debug?)
    exprs))
(defmacro printf [fmt args...]
  (print (format fmt args)))
(defnative get-version []
  "Returns the current version of the gelrt.")
(defnative debug? []
  "Returns whether or not this is a debug instance of gelrt.")
(defnative load-bindings [filename]
  "Opens the bindings from shared library at path [filename].")
(defnative format [pattern args...] ;; TODO: move to gel/ namespace
  "Returns a formatted String using the supplied [pattern] and [args...].")
(defnative print [value] ;; TODO: move to gel/ namespace
  "Prints the supplied [value] to the console.")
(defnative docs? [o]
  "Returns the docstring attached to the supplied Object [o].")
(defnative type? [o]
  "Returns the type of Object [o].")
(defnative sizeof [o]
  "Returns the size of Object [o] in bytes.")

(printf "gel v{}" (get-version))
(debug-only
  (print "debug mode enabled."))

(deftype Object
  (defnative hashcode [o]
    "Returns the HashCode of Object [o]."))

(deftype EventEmitter
  (defnative on [emitter event func]
    "")
  (defnative emit [emitter event data?]
    ""))

(deftype Buffer
  (defnative get-capacity [b]
    "Returns the capacity of Buffer [b].")
  (defnative get-length [b]
    "Returns the length of Buffer [b]."))

(deftype Timer
  (defnative create [on_tick timeout repeat]
    "Creates a new Timer on the current thread EventLoop.")
  (defnative start! [idx]
    "Starts Timer [idx].")
  (defnative stop! [idx]
    "Stops Timer [idx].")
  (defnative again! [idx]
    "Runs Timer [idx] again.")
  (defnative get-due-in [idx]
    "Returns the number of milliseconds Timer [idx] is due in.")
  (defnative get-repeat [idx]
    "Returns the repeat value of Timer [idx].")
  (defnative set-repeat! [idx repeat]
    "Sets the repeat value of Timer [idx] to [repeat]."))
(defmacro interval [on_tick repeat]
  (Timer:create on_tick 0 repeat))
(defmacro timeout [on_tick timeout]
  (Timer:create on_tick timeout 0))

(defnative get-class [s]
  "Returns the Class for Symbol [s].")
(defnative get-classes []
  "Returns the list of Classes registered.")
(deftype Class
  (defnative get-id [c]
    "Returns the ClassId for Class [c].")
  (defnative is-primitive? [c]
    "Returns true if Class [c] is a primitive Class.")
  (defnative get-fields [c]
    "Returns a list of Fields for Class [c].")
  (defnative get-procedures [c]
    "Returns the Procedures for Class [c]."))

(defnative get-namespace [s]
  "Returns the Namespace for Symbol [s].")
(defnative get-namespaces []
  "Returns the list of Namespaces.")
(deftype Namespace
  (defnative get-symbol [ns]
    "Returns the Symbol for Namespace [ns].")
  (defnative get-owner [ns]
    "Returns the owner of Namespace [ns].")
  (defnative get-macros [ns]
    "Returns the list of Macros for Namespace [ns].")
  (defnative get-procedures [ns]
    "Returns the list of Procedures for Namespace [ns].")
  (defnative get-lambdas [ns]
    "Returns the list of Lambdas for Namespace [ns].")
  (defnative get-native-procedures [ns]
    "Returns the list of NativeProcedures for Namespace [ns]."))

(deftype Macro
  (defnative get-owner [m]
    "Returns the owner if available for Macro [m].")
  (defnative get-symbol [m]
    "Returns the Symbol for Macro [m]."))

(defnative get-module [s]
  "Returns the Module for Symbol [s].")
(defnative get-modules []
  "Returns the list of Modules.")
(deftype Module
  (defnative is-kernel? [m]
    "Returns whether or not Module [m] is a kernel Module.")
  (defnative get-namespaces [m]
    "Returns the list of Namespaces for Module [m]."))

(defnative set-car! [p v] ;; TODO: create instruction
  "Sets the first value of Pair [p] to [v].")
(defnative set-cdr! [p v] ;; TODO: create instruction
  "Sets the second value of Pair [p] to [v].")

;; Random
(defnative random []
  "Returns a random Long.")
(defnative random:range [min max]
  "Returns a random Long in the range of [min] to [max].")

;; ---------------------------------------------------------------------------------
;; Event Loop
;; ---------------------------------------------------------------------------------
(defnative get-event-loop []
  "Returns the EventLoop for the current thread.")
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
  "[x] squared."
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
(defn filter [f seq]
  (cond (null? seq) seq
    (f (car seq)) (cons (car seq) (filter f (cdr seq)))
    (filter f (cdr seq))))
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
    (nth (cdr seq) (- n 1))))