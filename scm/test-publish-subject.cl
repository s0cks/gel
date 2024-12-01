(let ((topic (rx:publish-subject)) ; create a topic
      (numbers ; create an observable from the topic that is for only numbers
        (let:rx (:->Observable topic)
          (rx:filter number?))))
  ; print each string, any errors and when the topic is completed
  (let:rx (:->Observable topic)
    (rx:filter string?)
    (rx:subscribe
      (lambda (next)
        (print (format "next: {}" next)))
      (lambda (error)
        (print (format "error: {}" error)))
      (lambda ()
        (print "completed!"))))
  ; for each number print it and the value squared
  (let:rx numbers
    (rx:map
      (lambda (x)
        (list x (sq x))))
    (rx:subscribe
      (lambda (next)
        (print (format "found a number: {}, heres the sq: {}" (car next) (cadr next))))))
  ; print each even number
  (let:rx numbers
    (rx:filter even?)
    (rx:subscribe
      (lambda (next)
        (print (format "found an even number: {}" next)))))
  ; publish some values to the topic
  (let:rx topic
    (rx:publish 11) ; publish an odd number
    (rx:publish "Hello World") ; publish a string
    (rx:publish 10) ; publish an even number
    (rx:complete))) ; complete the topic/observable