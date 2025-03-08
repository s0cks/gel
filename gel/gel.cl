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
(defnative on-shutdown [func])
(defnative queue-utask [func])

(deftype EventEmitter
  (defnative on [emitter event func])
  (defnative emit [emitter event data?]))

(printf "gel v{}" (get-version))
(printf "this := {}" this)
(debug-only
  (print "debug mode enabled."))

(import "object.cl")
(import "events.cl")
(import "buffer.cl")
(import "timer.cl")
(import "class.cl")
(import "namespace.cl")
(import "map.cl")
(import "macro.cl")
(import "module.cl")

(defnative set-car! [p v] ;; TODO: create instruction
  "Sets the first value of Pair [p] to [v].")
(defnative set-cdr! [p v] ;; TODO: create instruction
  "Sets the second value of Pair [p] to [v].")
(defn apply [f seq]
  (cond (null? seq) seq
    (begin
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