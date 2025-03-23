(defmacro if [test body...]
  (cond test (do body)))
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
(defnative docs? [value]
  "Returns the docstring attached to the supplied Object [value].")
(defnative type? [value]
  "Returns the type of Object [value].")
(defnative sizeof [value]
  "Returns the size of Object [value] in bytes.")
(defnative on-shutdown [func])
(defnative queue-utask [func])

(defnative compare [x y])

(defn any? [x]
  "Returns true for any Object [x]."
  #t)
(defn pos? [n]
  "Returns true if Number [n] is positive."
  (> n 0))
(defn neg? [n]
  "Returns true if Number [n] is negative."
  (< n 0))
(defnative nan? [n]
  "Returns true if Number [n] equals NaN.")
(defnative abs [n]
  "Returns the abs value of Number [n].")
(defnative bit-str [n]
  "Returns the a String of bits for Number [n].")

(defn bit-test [n p]
  "Returns true if bit at position [p] is set in Number [n]."
  (Bool (bit-and n (bit-shl 1 p))))
(defn bit-set [n p]
  "Sets the bit at position [p] in Number [n]."
  (bit-or n (bit-shl 1 p)))
(defn bit-clear [n p]
  "Clears the bit at position [p] in Number [n]."
  (bit-and n (bit-not (bit-shl 1 p))))

(deftype EventEmitter
  (defnative on [emitter event func])
  (defnative emit [emitter event data?]))

(printf "gel v{}" (get-version))
(debug-only
  (print "debug mode enabled."))

(import "object.cl")
(import "events.cl")
(import "buffer.cl")
(import "timer.cl")
(import "class.cl")
(import "namespace.cl")
(import "map.cl")
(import "set.cl")
(import "macro.cl")
(import "module.cl")
(import "iterator.cl")

(defn apply [f seq]
  (cond (null? seq) seq
    (do
      (f (car seq))
      (apply f (cdr seq)))))

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

(import "utils.cl")