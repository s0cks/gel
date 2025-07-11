# Pairs

## Creating a Pair

```lisp
; Ex:
;   (x . y)
; or:
;   (cons x y)
(10 . 10)           ; => (10 . 10)
(cons 15 14)        ; => (15 . 14)
```

## first

```lisp
; (first) is a unary-expr that returns the first value of a pair:
(first (99 . 1))           ; => 99
```

## set-first

```lisp
; (set-first!) changes the (first) value of a pair to the supplied new value.
(do
  (def x (12 . 13))
  (set-first! x 14)
  (printf "x := {}" x))       ; => (14 . 13)
```

## second

```lisp
; (second) is a unary-expr that returns the second value of a pair:
(first (18 . 122))           ; => 122
```

## set-second

```lisp
; (set-second!) changes the (second) value of a pair to the supplied new value.
(do
  (def x (114 . 33))
  (set-second! x 10)
  (printf "x := {}" x))       ; => (114 . 10)
```
