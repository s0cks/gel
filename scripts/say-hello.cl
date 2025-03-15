(def message "Hello")
(defn say-hello [name]
  (def postfix "!!!")
  (printf "{} {} {}" message name postfix))
(say-hello "Lacey")

; (def events (EventEmitter))
; (events:on "hello"
;   $((say-hello "Lacey")))
; (queue-utask $((events:emit "hello")))

; (def filename "scripts/say-hello.cl")
; (printf "opening {}...." filename)
; (fs/open
;   filename
;   0
;   512
;   (fn [fd]
;     (printf "{} opened!" filename)
;     (queue-utask
;       $((print "Hello World Again"))))
;   (fn [error]
;     (printf "error opening {}: {}" filename error))
;   $((print "file open finished")))
; (queue-utask
;   $((print "next tick 2")))
; (print "finished")