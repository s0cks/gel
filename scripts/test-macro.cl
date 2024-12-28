(defmacro say [msg]
  (print msg))

(defmacro two []
  (+ 1 1))

(defmacro five []
  (+ (two) 3))

(defmacro debug-only [expr]
  (when (gel/debug?)
    expr))

(debug-only
  (print "Ran in debug mode"))

(newline)
(say "Hello World")
(say (+ 10 10 10))
(say (sq (five)))
(newline)

(debug-only
  (say "Goodbye"))