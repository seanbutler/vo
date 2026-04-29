# VO

A minimal, expression-oriented programming language. The name comes from *lingvo* — Esperanto for *language*.

## Philosophy

VO has one universal data structure: the **hash**. Objects, modules, namespaces, prototypes, and constructors are all hashes. There are no classes, no arrays, no loops as primitives — only hashes, callables, and recursion.

Everything is an expression. Blocks return their last value. There is no `return` keyword.

## Syntax at a glance

```
// declaration
name = value                  // immutable, untyped
name : type = value           // immutable, typed
name : type <- value          // mutable, typed
target <- new_value           // reassignment

// hash (object / module / prototype)
point = { x : int = 0  y : int = 0 }

// callable (function)
add = (a : int, b : int) { a + b }

// hash with constructor
Node = {
    value : int = 0
    next         = {}
    () = (v : int, n) {
        self.value <- v
        self.next  <- n
    }
}
node = Node(42, {})           // clones Node, calls ()

// conditional expression
? x > 0 { "positive" } { "non-positive" }

// member access
point.x
point.(key_expr)              // dynamic key

// hash iteration
data >> (k, v) { printf_s("%s\n", k) }

// import
@ "stdio.vo"

// foreign function binding
spec = { lib : string = "libc.so.6"  abi : string = "c"
         symbol : string = "puts"
         params = { p1 : string = "cstring" }
         returns : string = "int" }
puts = $$ spec
```

## Key features

- **Hash as universal primitive** — one data structure covers objects, modules, prototypes, and constructors
- **Expression-oriented** — every construct produces a value; no `return` keyword
- **First-class callables** — functions are values; closures capture their environment
- **Prototype-based OOP** — calling a hash clones it and invokes its `()` constructor slot
- **C FFI via `$$`** — bind and call C library functions directly
- **No reserved words** — only symbols; `@` for import, `?` for conditional, `>>` for iteration, `$$` for foreign binding

## Building

```sh
cd cpp
cmake -B build
cmake --build build
```

## Running

```sh
./build/vo program.vo
./build/vo program.vo --trace    # show token stream
```

## Example — Sieve of Eratosthenes

```
@ "stdio.vo"

empty = { is_empty : int = 1 }

Node = {
    is_empty : int = 0
    value    : int = 0
    next            = empty
    () = (v : int, n) { self.value <- v  self.next <- n }
}

range  = (lo : int, hi : int) {
    ? lo > hi { empty } { Node(lo, range(lo + 1, hi)) }
}

filter = (list, pred) {
    ? list.is_empty { empty } {
        ? pred(list.value) {
            Node(list.value, filter(list.next, pred))
        } {
            filter(list.next, pred)
        }
    }
}

sieve  = (list) {
    ? list.is_empty { empty } {
        p : int = list.value
        Node(p, sieve(filter(list.next, (n : int) { n % p != 0 })))
    }
}

print_list = (list) {
    ? list.is_empty { } {
        printf_i("%d\n", list.value)
        print_list(list.next)
    }
}

print_list(sieve(range(2, 50)))
```

## Source files

| Path | Contents |
|------|----------|
| `cpp/src/lexer/` | Tokeniser |
| `cpp/src/parser/` | Recursive-descent parser |
| `cpp/src/ast/` | AST node definitions |
| `cpp/src/interpreter/` | Tree-walking interpreter, FFI, environment |
| `cpp/stdio.vo` | `printf_s` / `printf_i` bindings |
| `cpp/cstdio16.vo` | C stdio descriptor library |
| `cpp/cstd12.vo` | C stdlib descriptor library |
| `cpp/ffi.vo` | FFI helper (`bind_one`, `bind_lib`) |
