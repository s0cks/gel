(def message "Hello")
(defn say-hello [name]
  (def postfix "!!!")
  (printf "{} {} {}" message name postfix))
(say-hello "Lacey")