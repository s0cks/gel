(let:rx (0 ... 10)
  (rx:skip 1)
  (rx:take-while
    (fn [x]
      (< x 6)))
  (rx:filter even?)
  (rx:map sq)
  (rx:subscribe print))