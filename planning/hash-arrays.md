---
status: PENDING
priority: (MUST, SOONER, MEDIUM, BROAD)
depends: callable-sigil.md (@ syntax settled first)
---

# Hash arrays

## Background

VO hashes use string identifier keys. Lua tables have a built-in hybrid array/hash
structure; VO does not. Arrays can be built on top of hashes, but two changes are
needed to make them clean and efficient.

## Syntax — no new syntax required

`.(expr)` already handles dynamic key access for any expression, including integer literals:

```vo
hash.(99)             // integer key — already valid, same parse path as hash.(k)
hash.(n)              // variable key
hash.(n + 1)          // computed key
hash.("name"+"part")  // constructed string key -> "namepart"
hash.name             // static string key (sugar for hash.("name"))
```

`hash.[99]` is not needed and would introduce `[` as a special case used nowhere else.
`hash.99` would require a parser special case (`.` currently expects `(` or Identifier).
`hash.(99)` is the right syntax — it reuses an existing parse path with no changes.

## Integer keys must be distinct from string keys

Currently all hash keys are `std::string`. `hash.(99)` with integer `99` would need
to coerce to `"99"`, making it identical to `hash.("99")`. That conflation is wrong —
integer-indexed array slots and string-named members should be separate namespaces.

**Solution: variant key type**

```cpp
using Key = std::variant<std::string, int64_t>;
```

- `hash.name` and `hash.("name")` -> string key `"name"`
- `hash.(99)` -> integer key `int64_t(99)`
- `hash.("99")` -> string key `"99"` (distinct from integer key `99`)

### Variant key overhead

Negligible for VO's use case:

| Cost | Detail |
|---|---|
| Size | `sizeof(variant<string, int64_t>)` ≈ `sizeof(string)` — dominated by string, no meaningful increase |
| Hash | One type-dispatch branch per lookup — negligible |
| Equality | One type check before comparison — negligible |
| Construction | Integer keys are *cheaper* than current coerce-to-string — no heap allocation |

## Storage — insertion-ordered vector

For insertion-ordered `>>` and integer key support, replace `std::unordered_map<std::string, Value>`
with a single ordered structure:

```cpp
std::vector<std::pair<Key, Value>>   // insertion order preserved
```

Lookup is O(n) linear scan — acceptable for typical VO hash sizes (small, named members).
For large dense arrays the stdlib `array.vo` layer manages access patterns.

Alternative considered: two separate maps (`unordered_map<string>` + `unordered_map<int64_t>`).
Rejected: `>>` iteration would need to merge two ranges and loses a single insertion order.

## `>>` iteration

`>>` visits all slots in insertion order, skipping `_`-prefixed and `()` slots as now.
Both string and integer keys are visited — `k` receives the key as its natural type
(string or int).

## Sparse vs dense

Setting a distant index is valid — gaps are left empty:

```vo
arr.(0)  := "a"
arr.(99) := "z"
// two slots exist; indices 1–98 don't — reading them returns {}
```

### `_size` — extent not count

`_size` = highest integer index + 1. Matches `loops.for(0, arr._size, ...)` expectation.

### Reading an unset index

```vo
arr.(50)   // slot not present -> returns {} (null)
```

Consistent with VO's existing null sentinel. No special case needed.

### `>>` on a sparse array

`>>` only visits keys that exist — gaps are skipped:

```vo
arr.(0)  := "a"
arr.(99) := "z"
arr >> @(k v) { }   // two iterations, not 100
```

For dense iteration (gaps included as `{}`) use `array.each` from `lib/array.vo`.

## Stdlib library (`lib/array.vo`)

```vo
array = {
    new = @(...) {
        // bind (0), (1), ... from args, set _size
    }

    set = @(a i v) {
        a.(i) := v
        ? i >= a._size { a._size := i + 1 }
    }

    get  = @(a i) { a.(i) }

    len  = @(a)   { a._size }

    each = @(a f) {
        loops.for(0, a._size, @(i) { f(i, array.get(a, i)) })
    }

    each_set = @(a f) {
        a >> @(k v) { ? k != "_size" { f(k, v) } }
    }

    push = @(a v) {
        a.(a._size) := v
        a._size := a._size + 1
    }
}
```

Two iteration modes:
- `array.each` — walks every index 0.._size (gaps appear as `{}`)
- `array.each_set` — uses `>>`, skips gaps, visits only defined slots

## Use cases

**Dense arrays:**
```vo
arr = array.new("a", "b", "c")
array.each(arr, @(i v) { printf_s("%s\n", v) })   // a, b, c
array.get(arr, 1)                                  // "b"
array.push(arr, "d")
array.len(arr)                                     // 4
```

**Sparse arrays:**
```vo
grid = {}
grid._size := 0
array.set(grid, 0,   "start")
array.set(grid, 999, "end")
array.each_set(grid, @(k v) { printf_s("%s\n", v) })  // "start", "end" only
```

## Performance note

Hash-backed arrays are O(1) average for random access (unordered_map) or O(n) with the
insertion-ordered vector. For typical VO use cases — game objects, config, moderate-sized
lists — this is fine. For tight inner-loop numeric data reach for a C FFI buffer.

## Implementation order

1. **Variant key type** — change hash storage from `std::string` key to `std::variant<std::string, int64_t>`; update all key lookup, hash, and equality paths
2. **Insertion-ordered storage** — replace `std::unordered_map` with `std::vector<std::pair<Key, Value>>`; update `>>` to iterate in order
3. **Interpreter: `.(expr)` integer dispatch** — when key expression evaluates to `int64_t`, store/lookup as integer key (not coerced to string)
4. **`lib/array.vo`** — pure VO, no C++ once steps 1–3 are done
