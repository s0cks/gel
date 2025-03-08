(ns gel
  (deftype Buffer
    (defnative read-uint8 [b idx])
    (defnative read-uint16 [b idx])
    (defnative read-uint32 [b idx])
    (defnative read-uint64 [b idx])

    (defnative write-uint8 [b val idx])
    (defnative write-uint16 [b val idx])
    (defnative write-uint32 [b val idx])
    (defnative write-uint64 [b val idx])

    (defnative fill [b val offset? length?])
    (defnative index-of [b val offset?])
    (defnative to-string [b encoding?])
    (defnative slice [b start? end?])
    (defnative get-capacity [b]
      "Returns the capacity of Buffer [b].")))