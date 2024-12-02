(def x 10)
(while (> x 0)
  (print (format "> {}" x))
  (set! x (- x 1)))