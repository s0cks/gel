(defmodule kernel
  (define sq (lambda (x) (* x x)))
  (define PI 3.14159)
  ; (define TAU (* 2 PI))
  (define even? (lambda (x) (eq? (% x 2) 0)))
  (define max (lambda (a b) (cond (> a b) a b)))
  (define min (lambda (a b) (cond (< a b) a b)))
  ; Types
  ; Numbers
  (define long? (lambda (x) (eq? (type? x) "Long")))
  (define double? (lambda (x) (eq? (type? x) "Double")))
  (define number? (lambda (x)
    (or (long? x) (double? x))))
  ; Strings
  (define string? (lambda (x) (eq? (type? x) "String")))
  (define symbol? (lambda (x) (eq? (type? x) "Symbol")))
  ; Procedures
  (define lambda? (lambda (x) (eq? (type? x) "Lambda")))
  (define native-procedure? (lambda (x) (eq? (type? x) "NativeProcedure")))
  (define procedure? (lambda (x)
    (or (native-procedure? x) (eq? (type? x) "Procedure"))))
  ; Misc
  (define pair? (lambda (x) (eq? (type? x) "Pair")))
  (define module? (lambda (x) (eq? (type? x) "Module")))

  ; foreach
  (define foreach (lambda (func seq)
    (loop
      (func (car seq))
      (set! seq (cdr seq))
      (when (null? seq)
        (return)))))

  ; map
  (define map (lambda (func seq)
    (print "Unavailable"))))