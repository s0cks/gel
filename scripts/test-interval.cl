(def x 0)
(interval
  (fn []
    (print (format "- #{}: Hello World" x))
    (set! x 1)
    (when (> x 5)
      (Timer:stop 1)))
  1000)