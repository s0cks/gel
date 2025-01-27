(gel/load-bindings "math")

(def PI 3.141592654)
(ns math
  (defnative acos [a])
  (defnative asin [a])
  (defnative atan [a])
  (defnative ceil [a])
  (defnative cos [a])
  (defnative cosh [a])
  (defnative floor [a])
  (defnative log [a])
  (defnative log10 [a])
  (defnative pow [a b])
  ; (def PI 3.141592654)
  (defnative round [a])
  (defnative sin [a])
  (defnative sinh [a])
  (defnative sqrt [a])
  (defnative tan [a])
  (defnative tanh [a])

  (defn to-radians [degs]
    "Converts radians to degrees."
    (/ (* degs PI) 180.0))
  (defn to-degrees [rads]
    "Converts degrees to radians."
    (* rads (/ 180.0 PI))))