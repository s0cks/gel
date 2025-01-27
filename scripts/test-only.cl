(def test #f)
(defn test-only [func]
  (when test
    (func)))
(test-only $((print "Hello World")))