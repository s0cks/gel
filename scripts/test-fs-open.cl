(import "fs.cl")
(def filename "scripts/test-fs-open.cl")
(fs/open
  filename
  0
  512
  (fn [fd]
    (printf "{} fd: {}" filename fd))
  (fn [error]
    (printf "error stat'ing file `{}`: {}" filename error)))
(fs/open
  "scripts/test-fs-open2.cl"
  0
  512
  (fn [fd]
    (printf "{} fd: {}" filename fd))
  (fn [error]
    (printf "error stat'ing file `{}`: {}" filename error)))
