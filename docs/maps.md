# maps

## Literal Maps

```lisp
; { k: v* }
; Ex:
{ test: true, message: "Hello World" }          ; => { test: true, message: "Hello World" }
; or
(do
  (def test_map { test: true, message: "Hello World"})
  (printf "map := {}" test_map))                ; => { test: true, message: "Hello World" }
```
