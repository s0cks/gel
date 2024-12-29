(gel/load-bindings "test-plugin")
(ns test
  (defnative say-hello []
    "Prints `Hello World` to the console."))