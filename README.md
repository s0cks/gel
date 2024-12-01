# gel

<!-- START doctoc -->
<!-- END doctoc -->

## Examples

see [gel/](gel/).

```lisp
(defun add-one (x)
  (+ x 1))
(let ((x (1 2 3))
      (y (1 (+ 1 1) 3))
      (z (1 (add-one 2) 3)))
  (print x)
  (print y)
  (print z))
```

Run:

```bash
gelrt \
  -logtostdout \
  -colorlogtostdout \
  --module-dir ./gel \
  gel/test-let.cl
```

Output:

```text
I20241201 14:45:34.007459 0x1f49cf240 runtime.cc:324] runtime initialized in 0.001 s
(1, (2, (3, ())))
(1, (2, (3, ())))
(1, (3, (3, ())))
I20241201 14:45:34.008189 0x1f49cf240 main.cc:32] finished in 6.6834e-05 s
```

### rx Example

```lisp
(let ((topic (rx:publish-subject)) ; create a topic
  (numbers ; create an observable from the topic that is for only numbers
    (let:rx (:->Observable topic)
      (rx:filter number?))))
  ; print each string, any errors and when the topic is completed
  (let:rx (:->Observable topic)
    (rx:filter string?)
    (rx:subscribe
      (lambda (next)
        (print (format "found a String: {}" next)))
      (lambda (error)
        (print (format "error: {}" error)))
      (lambda ()
        (print "completed!"))))
  ; for each number print it and the value squared
  (let:rx numbers
    (rx:map
      (lambda (x)
        (list x (sq x))))
    (rx:subscribe
      (lambda (next)
        (print (format "found a Number: {}, heres the sq: {}" (car next) (cadr next)))
        (when (even? (car next))
          (print (format "found an even Number: {}" (car next)))))))
  ; publish some values to the topic
  (let:rx topic
    (rx:publish 11) ; publish an odd number
    (rx:publish "Hello World") ; publish a string
    (rx:publish 10) ; publish an even number
    (rx:publish (new:Error "This is an error")) ; publish an Error
    (rx:complete))) ; complete the topic/observable
```

Run:

```bash
gelrt \
  -logtostdout \
  -colorlogtostdout \
  --module-dir ./gel \
  gel/test-publish-subject.cl
```

Output:

```text
I20241201 14:46:10.984060 0x1f49cf240 runtime.cc:324] runtime initialized in 0.001 s
"found a Number: 11, heres the sq: 121"
"found a String: Hello World"
"found a Number: 10, heres the sq: 100"
"found an even Number: 10"
"error: gel::Error(message=This is an error)"
I20241201 14:46:10.985326 0x1f49cf240 main.cc:32] finished in 0.000483333 s
```

## Building

### Prerequisites

You will need the following things to build:

- clang
- cmake>=3.29.3
- vcpkg

You can build by doing the following:

```bash
# git clone
cd gel/
vcpkg install
mkdir build/ && cd build/
cmake --build --preset XXX .. # debug, release, etc. See CMakePresets.json
```

Check whether or not the build was successful:

```bash
./gel --version
```

## Packages

- benchmark
- fmt
- gflags
- glog
- gtest
- tracy
- units
- reactiveplusplus
