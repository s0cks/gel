(gel/load-bindings "test")
(ns test
  (defnative say-hello []
    "Prints `Hello World` to the console."))