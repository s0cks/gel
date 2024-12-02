(defn add-one [x]
  (+ x 1))
(let ((x (1 2 3))
      (y (1 (+ 1 1) 3))
      (z (1 (add-one 2) 3)))
  (print x)
  (print y)
  (print z))