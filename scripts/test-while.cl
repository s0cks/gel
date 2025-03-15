(def x 10)
(while (> x 0)
  (printf "x := {}" x)
  (set! x (- x 1)))