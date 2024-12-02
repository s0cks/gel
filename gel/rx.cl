(ns rx
  ;; Observables
  (defnative map [o f]
    "Map a value in an observable to another value.")
  (defnative filter [o p]
    "Filter Observable [o] using the supplied Predicate [p].")
  (defnative first [o]
    "Return the first value in Observable [o].")
  (defnative last [o]
    "Return the last value in Observable [o].")
  (defnative skip [o n]
    "Skip [n] values in Observable [o].")
  (defnative take [o n]
    "Take [n] values from Observable [o].")
  (defnative take-while [o p]
    "Take values from Observable [o] while Predicate [p].")
  (defnative buffer [o n]
    "Buffer [n] values from Observable [o] into a list.")
  ;; Subjects
  (defnative publish [s next]
    "Publish the [next] value to Subject [s].")
  (defnative complete [s]
    "Complete Subject [s].")
  ;; Misc
  (defnative subscribe [source on_next on_error? on_complete?]
    "Subscribe to an Observable or Subject [source] using the supplied [on_next] and optional [on_error] & [on_complete]."))