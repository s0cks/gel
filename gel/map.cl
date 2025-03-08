(ns gel
  (deftype Map
    (defnative get [m k])
    (defnative contains [m k]
      "Returns true if Map [m] contains key [k].")
    (defnative keys [m]
      "Returns the keys of Map [m].")
    (defnative values [m]
      "Returns the values of Map [m].")
    (defnative size [m]
      "Returns the size of Map [m].")))