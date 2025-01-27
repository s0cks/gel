(defn say-hello [name]
  (print (format "Hello {}" name)))
(say-hello "Tazz")

(import "math.cl")
(printf "asin(0.10): {}" (math/asin 0.10))
(printf "to-radians(45.0): {}" (math/to-radians 45.0))