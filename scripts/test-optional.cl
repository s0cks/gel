(defn test [a b c?]
  (cond (null? c) (+ a b)
    (+ a b c)))
(print (format "- {}" (test 1 2)))
(print (format "- {}" (test 1 2 5)))

(defn test2 [a b c?]
  (print (format "a: {}" a))
  (print (format "b: {}" b))
  (print (format "c: {}" c))
  (newline))

(test2 10 11 12)
(test2 1 4)
(test2 5 4 3)