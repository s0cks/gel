(ns array
  (defnative array:get [a i]
    "Returns the value at index [i] for Array [a].")
  (defnative array:set! [a i v]
    "Sets the value at index [i] in Array [a] to [v].")
  (defnative array:length [a]
    "Returns the length of Array [a]."))