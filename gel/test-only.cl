(define test #f)
(defun test-only (func)
  (when test
    (func)))
(test-only (lambda () (print "Hello World")))