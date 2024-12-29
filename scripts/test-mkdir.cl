(import fs)
(def filename "./test")
(fs/mkdir filename 0755
  $((print (format "{} created!" filename)))
  $((print (format "error creating directory {}: {}" filename $)))
  $((print "finished creating directory")))