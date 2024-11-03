(defmodule kernel
  (define PI 3.14159)
  ; (define TAU (* 2 PI))
  (defun sq (x)
    (* x x))
  (defun even? (x)
    (eq? (% x 2) 0))
  (defun max (a b)
    (cond (> a b) a b))
  (defun min (a b)
    (cond (< a b) a b))
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
    (or (native-procedure? x) (eq? (type? x) "Procedure")))
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
  (defun foreach (f seq)
    (begin
      (define current seq)
      (while (not (null? current))
        (set! result (cons (f (car current)) result))
        (set! current (cdr current)))
      result))
  ; map
  (defun map (f seq)
    (begin
      (define result '())
      (define current seq)
      (while (not (null? current))
        (set! result (cons (f (car current)) result))
        (set! current (cdr current)))
      result)))