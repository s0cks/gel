(import "rx.cl")
(let:rx (0 ... 10)
  (rx/skip 1)
  (rx/take-while
    $((< $ 6)))
  (rx/filter
    $((eq? (% $ 2) 0)))
  (rx/subscribe
    print
    $((printf "error: {}" $))
    $((print "completed!"))))