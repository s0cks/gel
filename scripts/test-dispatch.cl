($((print $)) "Hello World")
((fn [$0]
  (print $0)) "Hello World")

($((print $) (print $)) "Hello" "World")
((fn [$0 $1]
  (print $0)
  (print $1)) "Hello" "World")

($((print (format "#1: {}; #0: {}" $1 $0))) "Hello" "World")
((fn [$0 $1]
  (print (format "#1: {}; #0: {}" $1 $0))) "Hello" "World")
