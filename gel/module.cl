(ns gel
  (defnative get-module [s]
    "Returns the Module for Symbol [s].")
  (defnative get-modules []
    "Returns the list of Modules.")
  (deftype Module
    (defnative is-kernel? [m]
      "Returns whether or not Module [m] is a kernel Module.")
    (defnative get-namespaces [m]
      "Returns the list of Namespaces for Module [m].")))