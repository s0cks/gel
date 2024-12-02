(ns rx
  ;; Observables
  (defnative map [observable func]
    "Map a value in an observable to another value.")
  (defnative filter [observable predicate]
    "Filter an observable using a supplied predicate.")
  (defnative first [observable]
    "")
  (defnative last [observable]
    "")
  (defnative skip [observable num]
    "")
  (defnative take [observable num]
    "")
  (defnative take-while [observable predicate]
    "")
  (defnative buffer [observable]
    "")
  ;; Subjects
  (defnative publish [subject valueOrError]
    "")
  (defnative complete [subject]
    "")
  ;; Misc
  (defnative subscribe [observableOrSubject]
    ""))