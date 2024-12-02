(defn add_one_with_docs [x]
  "Adds one to the supplied value and returns it"
  (+ x 1))
(defn empty [])
(defn empty_with_string []
  "Hello World")
(defn empty_with_string_and_docs []
  "Returns 'Hello World'"
  "Hello World")
(defn add_none [x] x)
(print (gel:docs? add_one_with_docs))
(print (gel:docs? empty))
(print (gel:docs? empty_with_string))
(print (gel:docs? empty_with_string_and_docs))
(print (gel:docs? add_none))