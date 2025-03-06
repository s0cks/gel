(gel/load-bindings "process")
(ns process
  (defnative get-cwd [])
  (defnative get-gid [])
  (defnative get-uid [])
  (defnative get-pid [])
  (defnative get-uptime []))