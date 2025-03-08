(ns gel
  (deftype Object
    (defnative hashcode [o]
      "Returns the HashCode of Object [o].")))