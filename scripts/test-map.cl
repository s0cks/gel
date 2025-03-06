(def m { test: "Hello World" })
(print (format "{} contains test: {}" m (m:contains 'test)))
(print (format "{} contains test2: {}" m (m:contains 'test2)))