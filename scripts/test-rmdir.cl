(import "fs.cl")
(def dirname "./test")
(fs/rmdir dirname
  $((printf "{} removed!" dirname))
  $((printf "error removing directory {}: {}" dirname $))
  $((print "finished removing directory!")))