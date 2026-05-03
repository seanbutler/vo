# VO

Vo is a small, expression-oriented programming language. The name comes from *lingvo* — Esperanto for *language*.

At its core, Vo provides a  one universal data structure: the **hash**. Objects, modules, namespaces, prototypes, and constructors are all hashes. There are no classes, no arrays — only hashes, callables, loops, and recursion.

Everything is an expression. Blocks return their last value. There is no `return` keyword.

## Syntax at a glance

```
// declaration
name = value                  // immutable, untyped
name : type = value           // immutable, typed
name : type := value          // mutable, typed
target := new_value           // reassignment

// identifiers — any UTF-8 sequence of letters, digits, _ and Unicode bytes
🐺 = "wolf"
speed🚀 : int := 0
café : int = 42

// hash (object / module / prototype)
point = { x : int = 0  y : int = 0 }

// callable (function)
add = (a : int, b : int) { a + b }

// hash with constructor
Node = {
    value : int = 0
    next         = {}
    () = (v : int, n) {
        self.value := v
        self.next  := n
    }
}
node = Node(42, {})           // clones Node, calls ()

// inheritance via _ delegation  (stdlib subtype helper)
@ "lib/stdlib.vo"
Animal = {
    sound : string = "..."
    speak  = () { self.sound }
    () = (s : string) { self.sound := s }
}
Dog = subtype(Animal, { sound : string = "Woof" })
d = Dog("Rex")                // constructor found through _ chain
d.speak()                     // method inherited; self = d

// private slots — _ prefix hides from >>, merge, clone, and display
Counter = {
    _count : int = 0
    inc    = () { self._count := self._count + 1 }
    value  = () { self._count }
}

// conditional expression (else branch optional, returns nil if absent)
? x > 0 { "positive" } { "non-positive" }
? x > 0 { "positive" }

// logical NOT
! x
! (a == b)

// loop block — repeats until \ is executed
~{
    ? done { \ }
    body
}

// four canonical loop forms
~{ ? !cond { \ }   body }         // while cond
~{ body   ? !cond { \ } }         // do-while cond
~{ ? done  { \ }   body }         // until done
~{ body }                          // infinite (exit via \ only)

// member access
point.x
point.(key_expr)              // dynamic key

// hash iteration  (skips _ prefixed and () slots)
data >> (k, v) { printf_s("%s\n", k) }

// import
@ "lib/stdio.vo"

// foreign function binding
spec = { lib : string = "libc.so.6"  abi : string = "c"
         symbol : string = "puts"
         params = { p1 : string = "cstring" }
         returns : string = "int" }
puts = $$ spec

// built-in functions
ifloor(3.7)               // → 3  (double → int, floor semantics)
char_at("hello", 1)       // → "e"  (single-character string at index)
```

## Key features

- **Hash as universal primitive** — one data structure covers objects, modules, prototypes, and constructors
- **Expression-oriented** — every construct produces a value; no `return` keyword
- **First-class callables** — functions are values; closures capture their environment
- **Prototype-based OOP** — calling a hash clones it and invokes its `()` constructor slot; `self` is bound in both constructors and methods
- **Inheritance via `_` delegation** — the `_` slot chains member lookup to a parent hash at runtime; `subtype` in stdlib builds child hashes with one call
- **Private slots** — any slot whose name begins with `_` is hidden from `>>`, `merge`, `clone`, and display; directly accessible by name
- **Loop primitive** — `~{ }` is an infinite loop block; `\` escapes it (lexically scoped, parse-time enforced); `!` is logical NOT
- **C FFI via `$$`** — bind and call C library functions directly
- **No reserved words** — only symbols; `@` import, `?` conditional, `~{ }` loop, `\` break, `!` not, `>>` iteration, `$$` FFI
- **Unicode identifiers** — any UTF-8 byte sequence is a valid identifier name, including emoji and extended-ASCII glyphs (e.g. `🐺 = "wolf"`, `speed🚀 : int := 0`, `café : int = 42`)
- **Built-in functions** — `ifloor(x)` truncates a double to int; `char_at(s, i)` returns the single-character string at index `i`

## Building

```sh
cd interp
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
@ "lib/stdio.vo"

empty = { is_empty : int = 1 }

Node = {
    is_empty : int = 0
    value    : int = 0
    next            = empty
    () = (v : int, n) { self.value := v  self.next := n }
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

## Example — Prototype OOP with inheritance

```
@ "lib/stdio.vo"
@ "lib/stdlib.vo"

// base — constructor + method
Animal = {
    sound : string = "..."
    speak  = () { printf_s("%s\n", self.sound) }
    () = (s : string) { self.sound := s }
}

// subtype inherits constructor and speak through _ chain
Dog = subtype(Animal, { sound : string = "Woof" })

// subtype with method override
Cat = subtype(Animal, {
    sound : string = "Meow"
    speak  = () { printf_s("Cat says: %s\n", self.sound) }
})

// multi-level inheritance
Poodle = subtype(Dog, { size : string = "small" })

a = Animal("Grunt")   a.speak()          // Grunt
d = Dog("Rex")        d.speak()          // Rex
c = Cat("Whiskers")   c.speak()          // Cat says: Whiskers
p = Poodle("Fifi")    p.speak()          // Fifi  (constructor through two _ hops)
```

Structs — hashes with no `()` — inherit methods the same way:

```
Point = {
    x : int = 0
    y : int = 0
    dot = (other) { self.x * other.x + self.y * other.y }
}

Point3 = subtype(Point, { z : int = 0 })

p = Point3()
p.x := 1  p.y := 2  p.z := 5
printf_i("%d\n", p.dot(p))   // 5
```

Private state with `_` prefix — hidden from `>>`, `merge`, and `clone`:

```
Counter = {
    _count : int = 0
    inc    = () { self._count := self._count + 1 }
    value  = () { self._count }
}

c = Counter()
c.inc()  c.inc()  c.inc()
printf_i("%d\n", c.value())   // 3
// c._count accessible directly but invisible to iteration
```

## Example — Extending the language via hashes

VO has no reserved words, so any vocabulary can be introduced as a plain hash of
callables. Two patterns:

### 1 — Aliasing: logic and loop vocabulary libraries

Two library hashes, each importable independently. No language changes required.

```
// lib/logic.vo
logic = {
    not = (x)    { !x }
    and = (a, b) { ? a() { b() } { 0 } }
    or  = (a, b) { ? a() { 1 }  { b() } }
}

// lib/loops.vo
loops = {
    while    = (cond, body) { ~{ ? !cond() { \ }  body() } }
    do_while = (body, cond) { ~{ body()  ? !cond() { \ } } }
    for      = (lo : int, hi : int, body) {
        i : int := lo
        ~{ ? i >= hi { \ }  body(i)  i := i + 1 }
    }
}

@ "lib/logic.vo"
@ "lib/loops.vo"

? logic.not(0) { printf_s("%s\n", "not(0) is true") }
? logic.and(() { 1 }, () { 1 }) { printf_s("%s\n", "1 and 1") }

i : int := 1
loops.while(() { i <= 5 }, () { printf_i("%d\n", i)  i := i + 1 })

total : int := 0
loops.for(1, 11, (n : int) { total := total + n })
printf_i("sum 1..10 = %d\n", total)    // 55
```

### 2 — New syntax: for loops as library callables

`lib/loops.vo` provides ascending, stepping, and descending for loops built on `~{ }`.

```
@ "lib/loops.vo"

loops.for(1, 6, (i : int) { printf_i("%d\n", i) })              // 1 2 3 4 5
loops.for_step(0, 11, 2, (i : int) { printf_i("%d\n", i) })     // 0 2 4 6 8 10
loops.for_down(1, 6, (i : int) { printf_i("%d\n", i) })         // 5 4 3 2 1
```

Full source: `interp/alias.vo`

## Source files

| Path | Contents |
|------|----------|
| `interp/src/lexer/` | Tokeniser |
| `interp/src/parser/` | Recursive-descent parser |
| `interp/src/ast/` | AST node definitions |
| `interp/src/interpreter/` | Tree-walking interpreter, FFI, environment |
| `interp/lib/stdio.vo` | `printf_s` / `printf_i` bindings |
| `interp/lib/stdlib.vo` | `clone`, `merge`, `subtype`, `without`, `has`, `size`, `rename`, `filter_map`, `map_values` |
| `interp/lib/metalib.vo` | Module interface helpers — `pick`, `omit`, `remap`, `public_api`, `exports_only` |
| `interp/lib/cstdio.vo` | C stdio descriptor library |
| `interp/lib/cstdlib.vo` | C stdlib descriptor library (`strlen`, `strcmp`, `strncmp`, `rand`, …) |
| `interp/lib/cstring.vo` | String utility hash `str` — `str.len`, `str.cmp`, `str.ncmp` (wraps cstdlib) |
| `interp/lib/cmath.vo` | C math descriptor library — `sin`, `cos`, `sqrt`, `pow`, `floor`, `ceil`, and more |
| `interp/lib/ffi.vo` | FFI helper (`bind_one`, `bind_lib`) |
| `interp/lib/logic.vo` | `logic` hash — `not`, `and`, `or` (lazy boolean) |
| `interp/lib/loops.vo` | `loops` hash — `while`, `do_while`, `for`, `for_step`, `for_down` |
| `interp/lib/vtkit.vo` | Terminal double-buffered rendering — `vtk` hash (`buffer_init`, `buffer_clear`, `buffer_vline`, `buffer_text`, `buffer_present`, colour constants, …) |
| `interp/alias.vo` | Example: using `logic` and `loops` together |

## Related languages

VO draws from several lineages. No single language shares all of its characteristics; the combination is what makes it distinct.

### Prototype cloning model

Calling a hash clones it and invokes its `()` slot — the core OOP mechanism.

| Language | Relationship |
|----------|-------------|
| **Self** | The origin of prototype cloning. Objects are cloned, slots are universal storage — the closest philosophical match to VO's hash model |
| **Io** | Everything is a message to a prototype; `Object clone` ≈ VO's `Hash()`. Minimal syntax, effectively no keywords |
| **NewtonScript** | Apple Newton PDA language; prototype cloning with a frame/slot model almost identical to VO hashes |
| **Lua** | Tables as universal structure; metatables for OOP — same philosophy, more ceremony |

### Expression-oriented / implicit return

Blocks return their last value; there is no `return` keyword.

| Language | Relationship |
|----------|-------------|
| **Ruby** | Last expression is the return value; blocks with `{}` |
| **CoffeeScript** | Implicit returns, cleaner JS semantics, `{}` object literals |
| **Rust** | Last expression returns; `let`/`let mut` mirrors VO's `=`/`:=` |
| **Scala** | Fully expression-oriented; type annotation syntax `name : Type` is identical to VO |
| **Haskell** | Everything is an expression; `<-` used for monadic binding |
| **MoonScript** | Implicit returns, compiles to Lua |

### `:=` mutable assignment operator

| Language | Relationship |
|----------|-------------|
| **Pascal / Ada** | `:=` is the assignment operator; `=` is comparison — the direct origin of VO's `:=` |
| **Go** | `:=` for short variable declaration with inferred type |
| **Algol** | The original source of `:=` as assignment |
| **Modula-2 / Oberon** | `:=` for assignment throughout |

### Type annotation syntax `name : type`

| Language | Relationship |
|----------|-------------|
| **Pascal / Ada** | The origin of the `name : type` convention |
| **Scala / Kotlin** | `val x : Int = 7` — nearly identical to VO |
| **Rust** | `let x : i32 = 7` — identical form |
| **TypeScript** | `const x : number = 7` — identical form |

### Symbol-only / no English keywords

| Language | Relationship |
|----------|-------------|
| **APL** | Entirely symbol-based; no English keywords at all — the extreme end of VO's direction |
| **J** | APL descendant; dense symbol vocabulary |
| **Rebol / Red** | No reserved words; everything is data; `[]` and `{}` as code — strong philosophical overlap |
| **Forth** | No keywords; all words are user-defined |

### Hash / map as the only data structure

| Language | Relationship |
|----------|-------------|
| **Lua** | Tables are everything — arrays, objects, modules — same unifying principle |
| **Clojure** | Maps as a core structure; everything is data |
| **Janet** | Lisp with first-class tables; lightweight and embeddable |
| **Tcl** | `{}` as code blocks; minimal distinctions between code and data |

### FFI design

VO's `$$` takes a hash descriptor — the binding spec is itself a first-class value.

| Language | Relationship |
|----------|-------------|
| **LuaJIT / FFI** | Closest match — C types declared as strings, called via `ffi.C.func()` |
| **Wren** | Foreign method binding via descriptors |
| **Python ctypes** | Spec-as-data approach to C binding |
| **Zig** | `@cImport` — compiler-level C interop via declarations |

