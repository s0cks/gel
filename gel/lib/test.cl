(dlopen "build/Sources/libtest-plugin.dylib")
(ns test
  (defnative say-hello []
    "Prints `Hello World` to the console."))