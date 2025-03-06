(def events (EventEmitter))
(events:on "test-event"
  (fn [data?]
    (print "on test-event")
    (when (nonnull? data)
      (print (format "data: {}" data)))))
(events:on "test-event"
  (fn [data?]
    (print "on test-event 2")))
(events:on "test-event2"
  (fn [data?]
    (print "on test-event2")))
(events:emit "test-event")
(events:emit "test-event" "test-event-data")
(events:emit "test-event2")
(print (format "events hashcode: {}" (events:hashcode)))
(def hello "Hello World")
(print (format "'{}' hashcode: {}" hello (hello:hashcode)))