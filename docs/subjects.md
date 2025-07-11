# Subjects

## Types of Subjects

### Publish Subjects

```lisp
; TODO: TBD
```

### Replay Subjects

```lisp
; TODO: TBD
```

## subscribe

```lisp
(do
  (def s (rx/publish-subject))
  (rx/subscribe s
    (fn [next]
      (print next))))
```
