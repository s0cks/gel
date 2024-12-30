($((print $)) "Hello World")
((fn [arg0]
  (print arg0)) "Hello World")

($((print $) (print $)) "Hello" "World")
((fn [arg0 arg1]
  (print arg0)
  (print arg1)) "Hello" "World")

($((print (format "#1: {}; #0: {}" $1 $0))) "Hello" "World")
((fn [arg0 arg1]
  (print (format "#1: {}; #0: {}" arg1 arg0))) "Hello" "World")
