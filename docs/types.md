# types

## primitive types

### nil

gel has a nil - or null, type:

```clojure
(def x nil)
;; you can also do:
(def x)
```

### bools

Bools represent one of two values: true, or false.

```clojure
(def my_bool true)
;; or:
(def my_bool false)
```

### strings

Strings represent a string of characters.

```clojure
(def my_string "Hello World")
;; or:
(def my_string 'This is a test string')
```

### numbers

Numbers can represent either any numeric value:

```clojure
(def x 10)
(def pi 3.141592654)
```

### pairs

Pairs represent two contiguous values. Similar to a pair - or tuple, in other languages.

```clojure
;; Pairs are defined like:
;;      ([first value], [second value])
;; Ex:
(def pos (10 . 10))
```

### functions

```clojure
;; Functions are defined like:
;;      (fn <name> [<param 1> <param 2> <param N>]
;;          "An example function"
;;          nil)
;; Ex:
(fn get_message[]
    "Returns a message to display to the user"
    "Hello World") ;; A function will always return the last value, in this case the string "Hello World"
;; When you have a function that doesnt return anything an implied nil will be returned
(fn do_work[]
    "This function does work but doesn't return anything"
    (print "doing work....")
    (do_more_work)
    (print "done"))
;; $ (do_work)
;; ->   doing work....
;;      done
;;      nil
```

### objects

Objects in gel are prototypal by nature and resemble objects in languages like Javascript

```clojure
(def my_object {})
```

### Object properties

All objects have their own properties - or fields

#### Getting properties

```clojure
;; You can get object properties using getf.
(def my_object {})
;; -> {}
;; In the event that a property doesnt exist in an object getf will return nil:
(getf my_object 'my-property')
;; -> nil
```

#### Setting properties

```clojure
;; You can set object properties using setf:
(def my_object {})
;; -> {}
(setf my_object 'message' 'Hello World')
;; -> nil
(getf my_object 'message')
;; -> "Hello World"
```
