(ns gel
  (defmacro caar [xs]
    (car (car xs)))
  (defmacro cadr [xs]
    (car (cdr xs)))
  (defmacro cdar [xs]
    (cdr (car xs)))
  (defmacro cddr [xs]
    (cdr (cdr xs)))
  (defmacro caaar [xs]
    (car (car (car xs))))
  (defmacro caadr [xs]
    (car (car (cdr xs))))
  (defmacro cadar [xs]
    (car (cdr (car xs))))
  (defmacro caddr [xs]
    (car (cdr (cdr xs))))
  (defmacro cdaar [xs]
    (cdr (car (car xs))))
  (defmacro cdadr [xs]
    (cdr (car (cdr xs))))
  (defmacro cddar [xs]
    (cdr (cdr (car xs))))
  (defmacro cdddr [xs]
    (cdr (cdr (cdr xs))))
  (defmacro caaaar [xs]
    (car (car (car (car xs)))))
  (defmacro caaadr [xs]
    (car (car (car (cdr xs)))))
  (defmacro caadar [xs]
    (car (car (cdr (car xs)))))
  (defmacro caaddr [xs]
    (car (car (cdr (cdr xs)))))
  (defmacro cadaar [xs]
    (car (cdr (car (car xs)))))
  (defmacro cadadr [xs]
    (car (cdr (car (cdr xs)))))
  (defmacro caddar [xs]
    (car (cdr (cdr (car xs)))))
  (defmacro cadddr [xs]
    (car (cdr (cdr (cdr xs)))))
  (defmacro cdaaar [xs]
    (cdr (car (car (car xs)))))
  (defmacro cdaadr [xs]
    (cdr (car (car (cdr xs)))))
  (defmacro cdadar [xs]
    (cdr (car (cdr (car xs)))))
  (defmacro cdaddr [xs]
    (cdr (car (cdr (cdr xs)))))
  (defmacro cddaar [xs]
    (cdr (cdr (car (car xs)))))
  (defmacro cddadr [xs]
    (cdr (cdr (car (cdr xs)))))
  (defmacro cdddar [xs]
    (cdr (cdr (cdr (car xs)))))
  (defmacro cddddr [xs]
    (cdr (cdr (cdr (cdr xs)))))
  (defmacro newline []
    "Prints a platform specific newline to the console."
    (print ""))
  (defmacro inc [x]
    "Returns one more than [x]."
    (+ x 1))
  (defmacro dec [x]
    "Returns one less than [x]."
    (- x 1))
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
      (nth (cdr seq) (- n 1)))))