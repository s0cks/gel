(def test #f)
(defn test-only [func]
  (when test
    (func)))
(test-only (fn [] (print "Hello World")))