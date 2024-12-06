; (import rx)
(defn add-one [v]
  (+ v 1))
(let ((x (1 2 3))
      (y (1 (+ 1 1) 3))
      (z (1 (add-one 1) 3)))
  (print x)
  (print y)
  (print z))
(def i
  (let ((values (0 ... 10)))
    (cadddr values)))
(print i)
(def j
  (let ((values (0 ... 10)))
    (print (format "values: {}" values))))
(print j)