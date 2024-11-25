;; pair accessors
(defun caar (xs)
  (car (car xs)))
(defun cadr (xs)
  (car (cdr xs)))
(defun cdar (xs)
  (cdr (car xs)))
(defun cddr (xs)
  (cdr (cdr xs)))
(defun caaar (xs)
  (car (car (car xs))))
(defun caadr (xs)
  (car (car (cdr xs))))
(defun cadar (xs)
  (car (cdr (car xs))))
(defun caddr (xs)
  (car (cdr (cdr xs))))
(defun cdaar (xs)
  (cdr (car (car xs))))
(defun cdadr (xs)
  (cdr (car (cdr xs))))
(defun cddar (xs)
  (cdr (cdr (car xs))))
(defun cdddr (xs)
  (cdr (cdr (cdr xs))))
(defun caaaar (xs)
  (car (car (car (car xs)))))
(defun caaadr (xs)
  (car (car (car (cdr xs)))))
(defun caadar (xs)
  (car (car (cdr (car xs)))))
(defun caaddr (xs)
  (car (car (cdr (cdr xs)))))
(defun cadaar (xs)
  (car (cdr (car (car xs)))))
(defun cadadr (xs)
  (car (cdr (car (cdr xs)))))
(defun caddar (xs)
  (car (cdr (cdr (car xs)))))
(defun cadddr (xs)
  (car (cdr (cdr (cdr xs)))))
(defun cdaaar (xs)
  (cdr (car (car (car xs)))))
(defun cdaadr (xs)
  (cdr (car (car (cdr xs)))))
(defun cdadar (xs)
  (cdr (car (cdr (car xs)))))
(defun cdaddr (xs)
  (cdr (car (cdr (cdr xs)))))
(defun cddaar (xs)
  (cdr (cdr (car (car xs)))))
(defun cddadr (xs)
  (cdr (cdr (car (cdr xs)))))
(defun cdddar (xs)
  (cdr (cdr (cdr (car xs)))))
(defun cddddr (xs)
  (cdr (cdr (cdr (cdr xs)))))
(define PI 3.14159)
; (define TAU (* 2 PI))
(defun sq (x)
  (* x x))
(defun zero? (x)
  (eq? x 0))
(defun even? (x)
  (zero? (% x 2)))
(defun max (a b)
  (cond (> a b) a
    b))
(defun min (a b)
  (cond (< a b) a
    b))
; Types
(defun null? (x)
  (eq? (type? x) "Null"))
; Numbers
(defun long? (x)
  (eq? (type? x) "Long"))
(defun double? (x)
  (eq? (type? x) "Double"))
(defun number? (x)
  (or (long? x) (double? x)))
; Strings
(defun string? (x)
  (eq? (type? x) "String"))
(defun symbol? (x)
  (eq? (type? x) "Symbol"))
; Procedures
(defun lambda? (x)
  (eq? (type? x) "Lambda"))
(defun native-procedure? (x)
  (eq? (type? x) "NativeProcedure"))
(defun procedure? (x)
  (or (native-procedure? x) (lambda? x) (eq? (type? x) "Procedure")))
; Misc
(defun pair? (x)
  (eq? (type? x) "Pair"))
(defun module? (x)
  (eq? (type? x) "Module"))
; factorial
(defun factorial (x)
  (cond (eq? x 1) 1
    (* x (factorial (- x 1)))))
; foreach
(defun apply (f seq)
  (cond (null? seq) seq
    (begin
      (f (car seq))
      (apply f (cdr seq)))))
; map
(defun map (f seq)
  (cond (null? seq) seq
    (cons (f (car seq)) (map f (cdr seq)))))
; filter
(defun filter (p seq)
 (cond (null? seq) seq
   (p (car seq)) (cons (car seq) (filter p (cdr seq)))
   (filter p (cdr seq))))
; length
(defun length (seq)
  (cond (null? seq) 0
    (+ 1 (length (cdr seq)))))
; append
(defun append (seq x)
  (cond (null? seq) x
    (cons (car seq) (append (cdr seq) x))))
; reverse
(defun _reverse (seq acc)
  (cond (null? seq) acc
    (_reverse (cdr seq) (cons (car seq) acc))))
(defun reverse (seq)
  (_reverse seq '()))
; nth
(defun nth (seq n)
  (when (or (> n (length seq)) (< n 0))
    (throw (format "Index `{}` out of bounds" n)))
  (cond (eq? n 0) (car seq)
    (nth (cdr seq) (- n 1))))