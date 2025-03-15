(defn pretty-print [x]
  (cond
    (eq? x 10) (print "ten")
    (eq? x 11) (print "elven")
    (eq? x 12)
      (begin
        (print "twelve")
        (print "twelve is even"))
    (print "idk")))
(pretty-print 10)
(pretty-print 11)
(pretty-print 12)
(pretty-print 13)