(ns gel
  (defnative union [a b])
  (defnative difference [a b])
  (defnative intersection [a b])
  (defnative subset? [a b])
  (defnative superset? [a b])
  (defnative select [set filter])
  (deftype Set
    (defnative contains? [s v])
    (defnative count [s])
    (defnative empty? [s])))