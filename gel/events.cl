(ns gel
  (deftype EventEmitter
    (defnative on [emitter event func])
    (defnative emit [emitter event data?])))