(ns gel
  (deftype Timer
    (defnative create [on_tick timeout repeat]
      "Creates a new Timer on the current thread EventLoop.")
    (defnative start! [idx]
      "Starts Timer [idx].")
    (defnative stop! [idx]
      "Stops Timer [idx].")
    (defnative again! [idx]
      "Runs Timer [idx] again.")
    (defnative get-due-in [idx]
      "Returns the number of milliseconds Timer [idx] is due in.")
    (defnative get-repeat [idx]
      "Returns the repeat value of Timer [idx].")
    (defnative set-repeat! [idx repeat]
      "Sets the repeat value of Timer [idx] to [repeat]."))
  (defmacro interval [on_tick repeat]
    (Timer:create on_tick 0 repeat))
  (defmacro timeout [on_tick timeout]
    (Timer:create on_tick timeout 0)))