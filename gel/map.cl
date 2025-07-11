(ns gel
  (deftype map
    (defnative get [m k])
    (defnative contains [m k]
      "Returns true if map [m] contains key [k].")
    (defnative keys [m]
      "Returns the keys of map [m].")
    (defnative values [m]
      "Returns the values of map [m].")
    (defnative size [m]
      "Returns the size of map [m].")))