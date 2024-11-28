(let ((values (1 2 3 4)))
  (let:rx (rx:to-observable values)
    (rx:map (lambda (x) (+ x 1)))
    (rx:subscribe print)))