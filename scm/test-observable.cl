(defun sq (x)
  (* x x))
(let:rx (1 2 3 4 5 6)
  (rx:take_while (lambda (x) (< x 6)))
  (rx:map sq)
  (rx:subscribe print))