(defmodule kernel
  (define even? (lambda (x) (eq? (% x 2) 0)))
  (define number? (lambda (x) (eq? (type? x) "Number")))
  (define string? (lambda (x) (eq? (type? x) "String")))
  (define symbol? (lambda (x) (eq? (type? x) "Symbol")))
  (define lambda? (lambda (x) (eq? (type? x) "Lambda")))
  (define procedure? (lambda (x) (eq? (type? x) "Procedure"))))