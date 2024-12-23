(defn say-hello [name]
  (print (format "Hello {}" name))
  (gel:print-st))
(say-hello "Tazz")
(print (format "locals: {}" (gel:get-locals)))
(newline)
(print (gel:docs? gel:docs?))
(newline)
(print (gel:docs? random:range))