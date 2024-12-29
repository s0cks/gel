(gel/load-bindings "libtest-plugin.dylib")
(ns test
  (defnative say-hello []
    "Prints `Hello World` to the console."))