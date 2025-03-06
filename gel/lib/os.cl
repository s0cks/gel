(ns os
  (defnative get-arch [])
  (defnative get-cpus [])
  (defnative get-platform [])
  (defnative get-version [])
  (defnative get-hostname [])
  (defnative get-total-mem [])
  (defnative get-free-mem [])
  (defnative get-uptime []))