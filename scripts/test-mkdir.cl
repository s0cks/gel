(import "fs.cl")
(def dirname "./test")
(printf "creating {}..." dirname)
(fs/mkdir dirname 0755
  (fn [] (printf "{} created!" dirname))
  (fn [error?] (printf "error creating directory {}: {}" dirname error))
  (fn [] (print "finished creating directory")))