(gel/load-bindings "math")
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
  (defnative round [a])
  (defnative sin [a])
  (defnative sinh [a])
  (defnative sqrt [a])
  (defnative tan [a])
  (defnative tanh [a])

  (def PI 3.141592654)
  (def TAU (* PI 2.0))
  (def TWENTY (* 10 2))

  (defn to-radians [degs]
    "Converts radians to degrees."
    (/ (* degs PI) 180.0))
  (defn to-degrees [rads]
    "Converts degrees to radians."
    (* rads (/ 180.0 PI))))