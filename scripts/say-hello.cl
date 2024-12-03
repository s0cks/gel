(defn say-hello [name]
  (print (format "Hello {}" name)))
(say-hello "Tazz")
(print (format "locals: {}" (gel:get-locals)))
(print "")
(print (gel:docs? gel:docs?))
(print "")
(print (gel:docs? random:range))