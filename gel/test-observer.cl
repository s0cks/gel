
(let ((observer (new:Observer
                  (lambda (next)
                    (print (format "next: {}" next)))
                  (lambda (error)
                    (print (format "error: {}" error)))
                  (lambda ()
                    (print "completed")))))
  (let:rx (1 ... 10)
    (rx:filter even?)
    (rx:subscribe observer)))