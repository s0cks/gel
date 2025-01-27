(timeout
  (fn []
    (print "Hello World"))
  2000)
(printf "repeat: {}" (Timer:get-repeat 0))
(printf "due-in: {}" (Timer:get-due-in 0))