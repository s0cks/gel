
(let ((observer (new:Observer
                  (fn [next]
                    (print (format "next: {}" next)))
                  (fn [error]
                    (print (format "error: {}" error)))
                  (fn []
                    (print "completed")))))
  (let:rx (1 ... 10)
    (rx:filter even?)
    (rx:subscribe observer)))