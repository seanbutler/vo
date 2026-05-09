---
status: PENDING
priority: (MUST, SOONER, MEDIUM, BROAD)
depends: callable-sigil.md (@ syntax settled first)
---

# Hash arrays

## Background

VO hashes use string identifier keys. Lua tables have a built-in hybrid array/hash
structure; VO does not. Arrays can be built on top of hashes, but two language changes
are needed to make them clean and efficient.

## Required language changes

### 1. Insertion-ordered `>>`

Currently `>>` iterates via `std::unordered_map` — no order guarantee. Switch to an
insertion-ordered map (e.g. `std::map` sorted, or a linked hash map). `>>` then gives
predictable sequential iteration on any hash with no new syntax:

```vo
arr = { x = "a"  y = "b"  z = "c" }
arr >> @(k v) { printf_s("%s\n", v) }   // a, b, c — guaranteed insertion order
```

Solves ordered iteration. Does not solve random access.

### 2. Bracket key syntax `[expr]`

Allow any expression as a hash key in literals and as an assignment target:

```vo
arr = { [0] = "a"  [1] = "b"  [2] = "c" }
arr.[1]          // "b" — O(1) direct hash lookup
arr.[i]          // dynamic read
arr.[99] := "x"  // dynamic write — bracket syntax on assignment target
```

Keys stored internally as strings (`"0"`, `"1"`, ...). `>>` sorts integer-looking keys
numerically before string keys (JavaScript-style).

**Parser changes needed:**
- `[expr]` as a hash literal member key
- `hash.[expr]` as a read expression (postfix, alongside existing `hash.(expr)`)
- `hash.[expr] := value` as a valid assignment target

## Sparse vs dense

Setting a distant index directly is valid — it just leaves gaps:

```vo
arr.[0]  := "a"
arr.[99] := "z"
// hash has two slots: "0" and "99"
// indices 1–98 don't exist — reading them returns {} (VO's null)
```

This is a **sparse array** by default. Only pay for what you store.

### `_size` ambiguity

For sparse arrays, two meanings of "size" diverge:

| Convention | `_size` means | Useful for |
|---|---|---|
| Extent | highest index + 1 (= 100 above) | dense-style loops, `loops.for(0, _size, ...)` |
| Count  | number of set slots (= 2 above) | memory accounting, sparse iteration |

**Decision: use extent.** `_size` = highest index + 1. Matches the expectation that
`loops.for(0, array.len(arr), ...)` visits every logical position. Gaps are visited as
`{}`.

### Reading an unset index

```vo
arr.[50]   // slot "50" not present → returns {} (null)
```

Consistent with VO's existing null sentinel. No special case needed.

### `>>` on a sparse array

`>>` only visits keys that exist — gaps are skipped:

```vo
arr.[0]  := "a"
arr.[99] := "z"
arr >> @(k v) { }   // visits "a" then "z" only — two iterations, not 100
```

This is correct for sparse use cases. For dense iteration use `array_each` (see below).

## Stdlib library (`lib/array.vo`)

```vo
array = {
    new = @(...) {
        // bind [0], [1], ... from args, set _size
    }

    set = @(a i v) {
        a.(i) := v
        ? i >= a._size { a._size := i + 1 }  // track extent
    }

    get  = @(a i) { a.(i) }     // returns {} if unset — caller checks

    len  = @(a)   { a._size }   // extent: highest index + 1

    // dense iteration — visits every index 0.._size, gaps included as {}
    each = @(a f) {
        loops.for(0, a._size, @(i) { f(i, array.get(a, i)) })
    }

    // sparse iteration — skips gaps, only visits set slots
    each_set = @(a f) {
        a >> @(k v) { ? k != "_size" { f(k, v) } }
    }

    push = @(a v) {
        a.(a._size) := v
        a._size := a._size + 1
    }
}
```

Two iteration modes — caller picks the right one:
- `array.each` — walks every index 0.._size (gaps appear as `{}`)
- `array.each_set` — uses `>>`, skips gaps, visits only defined slots

## Use cases

**Dense arrays** (insertion order + bracket keys):
```vo
arr = array.new("a", "b", "c")
array.each(arr, @(i v) { printf_s("%s\n", v) })   // a, b, c
array.get(arr, 1)                                  // "b"
array.push(arr, "d")
array.len(arr)                                     // 4
```

**Sparse arrays** (entity-component, adjacency maps, most-indices-empty):
```vo
grid = {}
grid._size := 0
array.set(grid, 0,   "start")
array.set(grid, 999, "end")
array.each_set(grid, @(k v) { printf_s("%s\n", v) })  // "start", "end" only
```

## Performance note

Hash-backed arrays are O(1) average for random access but have higher constant cost
than contiguous memory (pointer chase + key comparison). For most VO use cases —
game objects, config, moderate-sized lists — this is acceptable.

For tight inner-loop numeric data (image buffers, large matrices) reach for a C FFI
buffer instead. The hash array is not a replacement for a typed contiguous array.

## Implementation order

1. Insertion-ordered `>>` — interpreter change (`std::unordered_map` → ordered map)
2. Bracket key syntax `[expr]` — lexer + parser change (read + write positions)
3. `lib/array.vo` — pure VO, no C++ once steps 1 and 2 are done
