# Unary Expressions

## Not

```lisp
(not <expr>)
; Ex:
(not true)        ; => false
(not 10)          ; => false
(not false)       ; => true
```

## First

```lisp
(first <expr>)
; Ex:
(first (10 . 10))         ; => 10
(first (1 2 3))           ; => 1
(first (1...10))          ; => 1
```

## Second

```lisp
(second <expr>)
; Ex:
(second (10 . 10))        ; => 10
(second (1 2 3))          ; => (2 3)
(second (1...10))         ; => (2 3 4 5 6 7 8 9 10)
```

## Nonnull

```lisp
(nonnull? <expr>)
; Ex:
(nonnull? true)                ; => true
(nonnull? null)                ; => false
(nonnull? "Hello World")       ; => true
```

## Null

```lisp
(null? <expr>)
; Ex:
(null? true)                ; => false
(null? null)                ; => true
(null? "Hello World")       ; => false
```

## Bitwise Not

```lisp
(bnot <expr>)
; Ex:
(bnot 0b110000) ; => (0b001111)
```
