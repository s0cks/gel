(ns gel
  (deftype Macro
    (defnative get-owner [m]
      "Returns the owner if available for Macro [m].")
    (defnative get-symbol [m]
      "Returns the Symbol for Macro [m].")))