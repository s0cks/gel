(import rx)
(let:rx (0 ... 10)
  (rx/skip 1)
  (rx/take-while
    (fn [x]
      (< x 6)))
  (rx/filter
    (fn [x]
      (eq? (% x 2) 0)))
  (rx/map
    (fn [x]
      (cons x (sq x))))
  (rx/subscribe
    print
    (fn [e]
      (print (format "error: {}" e)))
    (fn []
      (print "complete!"))))