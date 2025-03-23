(ns gel
  (deftype Iterator
    (defnative has-next? [iter])
    (defnative next [iter])))