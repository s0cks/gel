(defn say-hello [name]
  (print (format "Hello {}" name)))
(say-hello "Tazz")

(import "math.cl")
(printf "twenty: " (math/TWENTY))
(printf "tau: {}" (math/TAU))