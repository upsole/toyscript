# Toyscript
Interpreted language I built in order to learn. 
It comes with builtin support for lists, strings and first-class functions.

* Running `toyscript` without any arguments will start the repl. It will evaluate a file if a filename is provided.
* `build.sh` also allows for the tests to be run: `./build.sh test` will run all of them, `parser`, `lexer` or `eval` to run just  
a section. If you append a test number after one of those, it will run just that single test.
* See `sources` for toyscript examples
```
val range = fn(begin, end) { # Generate a range of integers
        var new_list = [];
        val recur = fn(x) {
                push(new_list, x);
                if (x < end) {
                        recur(x + 1);
                }
        }
        recur(begin);
        return new_list;
}

val filter = fn(list, predicate) { # Functions are first-class, we pass them around
        var res = [];
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
* Adding support for maps, tuples and some extra builtins to help with those.
* Some extra features like more parser information (ie: line number) should be didactic to implement...
