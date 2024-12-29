(gel/load-bindings "env")
(ns env
  (defnative get [key]
    "Returns the value from the environment variable bound to [key]."))