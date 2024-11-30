(let ((topic (rx:publish-subject))
      (is_string
        (lambda (x)
          (eq? (type? x) "String"))))
  (let:rx topic
    (rx:filter is_string)
    (rx:subscribe
      print
      (lambda (error)
        (print (format "error: {}" error)))
      (lambda ()
        (print "completed!"))))
  (rx:publish topic 11)
  (rx:publish topic "Hello World")
  (rx:publish topic 10)
  (rx:complete topic))