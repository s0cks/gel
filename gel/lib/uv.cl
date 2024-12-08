(ns uv
  "libuv bindings lib."
  (defnative alive [loop]
    "Returns non-zero if there is active work in the Loop [loop].")
  (defnative close [loop]
    "Closes the Loop [loop]. Note: Only call after all work in Loop [loop] has finished.")
  (defnative stop [loop]
    "Stops the Loop [loop].")
  (defnative run [loop]
    "Runs the Loop [loop]."))