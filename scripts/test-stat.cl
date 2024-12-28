(import fs)
(def filename "scripts/test-stat.cl")
(fs/stat
  filename
  (fn [size]
    (print (format "{} size: {}" filename size)))
  (fn [error]
    (print (format "error stat'ing file `{}`: {}" filename error))))