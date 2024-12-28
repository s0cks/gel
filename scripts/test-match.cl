(print
  (match (type? 10)
    ;; String types
    ('String
      (format "{} is a String" x))
    ;; Unknown types
    (format "no idea what the type of {} is" x)))