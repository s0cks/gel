(defn print_with_type [x]
  (print (format "{} ; {}" x  (type? x))))
(let ((s (rx:publish-subject)))
  (rx:subscribe
    s
    (fn [x] x)
    (fn [err]
      (print (format "error: {}" err)))
    (fn []
      (print "completed")))
  (let:rx (:->Observable s)
    (rx:filter even?)
    (rx:map sq)
    (rx:subscribe print_with_type))
  (let:rx (:->Observable s)
    (rx:filter
      (fn [x]
        (#String? x)))
    (rx:subscribe print_with_type))
  (rx:publish s "Hello World")
  (rx:publish s 2)
  (rx:complete s))