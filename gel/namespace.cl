(ns gel
  (defnative get-namespace [s]
    "Returns the Namespace for Symbol [s].")
  (defnative get-namespaces []
    "Returns the list of Namespaces.")
  (deftype Namespace
    (defnative get-symbol [ns]
      "Returns the Symbol for Namespace [ns].")
    (defnative get-owner [ns]
      "Returns the owner of Namespace [ns].")
    (defnative get-macros [ns]
      "Returns the list of Macros for Namespace [ns].")
    (defnative get-procedures [ns]
      "Returns the list of Procedures for Namespace [ns].")
    (defnative get-lambdas [ns]
      "Returns the list of Lambdas for Namespace [ns].")
    (defnative get-native-procedures [ns]
      "Returns the list of NativeProcedures for Namespace [ns].")))