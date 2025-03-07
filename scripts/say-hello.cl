(def message "Hello")
(defn say-hello [name]
  (printf "{} {}" message name))
(say-hello "Tazz")