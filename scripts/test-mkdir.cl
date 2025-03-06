(import "fs.cl")
(def dirname "./test")
(printf "creating {}..." dirname)
(fs/mkdir dirname 0755
  $((print (format "{} created!" dirname)))
  $((print (format "error creating directory {}: {}" dirname $)))
  $((print "finished creating directory")))