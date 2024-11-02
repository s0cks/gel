(begin
  (define factorial (lambda (x)
    (cond (eq? x 1) 1
      (* x (factorial (- x 1))))))
  (define y 5)
  (print (format "{}! equals: {}" y (factorial y))))