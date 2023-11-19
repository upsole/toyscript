# Toyscript
Interpreter I built in order to learn. 
It comes with builtin support for Lists and some functional programming support.

* Running `toyscript` without any arguments will start the repl. It will evaluate a file if a filename is provided.
* `build.sh` also allows for the tests to be run: `./build.sh test` will run all of them, `parser`, `lexer` or `eval` to run just  
a section. If you append a test number after one of those, it will run just that single test.
* See `tests/toy_sources` for toyscript examples
```
val range = fn(begin, end) {
        val new_list = [];
        val recur = fn(x) {
                push(new_list, x);
                if (x < end) {
                        recur(x + 1);
                }
        }
        recur(begin);
        return new_list;
}

val filter = fn(list, predicate) {
        val res = [];
        val recur = fn(tmp, item) {
                if (predicate(item)) {
                        push(res, item);
                }
                if (tmp) {
                        recur(cdr(tmp), car(tmp));
                }
        };
        recur(cdr(list), car(list));
        return res;
}
val even? = fn(x) {
        if ((x % 2) == 0) {true} else {false}
}
print(filter(range(0, 105), even?));
```

## To improve
* I would like to improve the memory model, currently it's intentionally left simple - it will leak during recursive calls, as the allocations for inner varibles to a function are not released.
* Adding support for hash maps should be trivial
* Some extra features like more parser information (ie: line number) should be didactic to implement...
