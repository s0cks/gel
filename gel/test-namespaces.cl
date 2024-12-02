(ns empty)
(ns empty_with_docs
  "This is an empty namespace with docs.")
(ns test_with_no_docs
  (defun print_sq (x)
    "Prints the sq of the supplied value."
    (cond (not (number? x)) '()
      (print (sq x)))))
(ns test_with_docs
  "This is a test namespace."
  (defun print_sq (x)
    "Prints the sq of the supplied value."
    (cond (not (number? x)) '()
      (print (sq x)))))
(test_with_no_docs:print_sq 10)
(test_with_docs:print_sq 10)