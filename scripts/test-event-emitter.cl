(def events (EventEmitter))
(events:on "test-event"
  (fn [data?]
    (print "on test-event")
    (when (nonnull? data)
      (printf "data: {}" data))))
(events:on "test-event"
  (fn [data?]
    (print "on test-event 2")))
(events:on "test-event2"
  (fn [data?]
    (print "on test-event2")))
(events:emit "test-event")
(events:emit "test-event" "test-event-data")
(events:emit "test-event2")
(printf "events hashcode: {}" (events:hashcode))
(def hello "Hello World")
(printf "'{}' hashcode: {}" hello (hello:hashcode))