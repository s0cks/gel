(ns _kernel
  "The main namespace for gel."
  ;; pair accessors
  (defn caar [xs]
    (car (car xs)))
  (defn cadr [xs]
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
      (array? seq)
        (begin
          (defn next [f seq idx]
            (cond (> idx (- (array:length seq) 1)) '()
              (begin
                (f (array:get seq idx))
                (next f seq (+ idx 1)))))
          (next f seq 0))
      (begin
        (f (car seq))
        (apply f (cdr seq)))))
  (defn map [f seq]
    (cond (null? seq) seq
      (array? seq)
        (begin
          (var idx 0)
          (var result '())
          (print "iterating")
          (while (< idx (array:length seq))
            (const item (f (array:get seq idx)))
            (set! result (cons item result))
            (set! idx (+ idx 1))
            (print (format "idx: {}" idx)))
          (print (format "result: {}" result))
          result) ; everything after the while doesn't get executed
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
  (defn reverse [seq]
    "Returns the reverse of a sequence."
    (defn _reverse [seq acc]
      (cond (null? seq) acc
        (_reverse (cdr seq) (cons (car seq) acc))))
    (_reverse seq '()))
  (defn nth [seq n]
    "Returns the nth value in a sequence."
    (when (or (> n (length seq)) (< n 0))
      (throw (format "Index `{}` out of bounds" n)))
    (cond (eq? n 0) (car seq)
      (nth (cdr seq) (- n 1)))))