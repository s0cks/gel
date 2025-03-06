(import "fs.cl")
(def dirname "./test")
(fs/rmdir dirname
  $((print (format "{} removed!" dirname)))
  $((print (format "error removing directory {}: {}" dirname $)))
  $((print "finished removing directory!")))