---
status: PENDING
priority: (COULD, SOONER, MEDIUM, BROAD)
---

# Replace hardcoded NativeCallable builtins with FFI + external libs

`ifloor` and `char_at` are currently special-cased as `NativeCallable` objects wired directly into the interpreter in `register_builtins()`. They exist because the FFI dispatcher can't express their signatures.

The goal is to remove them and use ordinary `bind_lib`-bound C functions.

## What is blocking them today

| Builtin | Blocker |
|---|---|
| `ifloor(double) → int` | No `double` param type in the integer-register-class dispatcher |
| `char_at(string, int) → string` | No `cstring` return type — dispatcher can only return `int64_t` |

## Step 1 — extend the FFI dispatcher (`interpreter.cpp`)

- Add `"double"` as a recognised param type in the integer-register-class path (reinterpret `double` bits to `int64_t` for passing, interpret result accordingly)
- Add `"cstring"` as a return type: cast the `int64_t` result to `const char*` and wrap in `Value::from(std::string(...))`

## Step 2 — C shim library (`interp/vendor/vostr/vostr.c`, compiled to `libvostr.so`)

```c
#include <math.h>
#include <string.h>

// char_at: returns pointer to a 1-char null-terminated static buffer.
// Safe for immediate use; caller must not store the pointer across calls.
static char vo_char_at_buf[2];
const char* vo_char_at(const char* s, int index) {
    vo_char_at_buf[0] = s[index];
    vo_char_at_buf[1] = '\0';
    return vo_char_at_buf;
}

int vo_ifloor(double x) { return (int)floor(x); }
```

## Step 3 — VO descriptor (`interp/lib/cstring.vo` and `interp/lib/cmath.vo`)

```vo
// cstring.vo — char_at replacement
vostr_lib = { lib : string = "libvostr.so"  abi : string = "c"
    functions = {
        char_at = { symbol : string = "vo_char_at"
                    params  = { p1 : string = "cstring"  p2 : string = "int" }
                    returns : string = "cstring" }
    }
}
vostr = bind_lib(vostr_lib)
char_at = @(s : string  i : int) { vostr.char_at(s, i) }
```

```vo
// in cmath.vo — add alongside sin/cos/etc:
        ifloor = { symbol : string = "vo_ifloor"
                   params  = { p1 : string = "double" }
                   returns : string = "int" }
```

## Step 4 — remove from interpreter

- Delete the `ifloor` and `char_at` blocks from `Interpreter::register_builtins()`
- Update `lib/cstring.vo` comment (currently reads "char_at is a language builtin, not listed here")

## Result — usage unchanged from caller perspective

```vo
# "lib/cmath.vo"
# "lib/cstring.vo"

row : string = "1011100001111001"
cell : string = char_at(row, 3)        // "1"
y : int = ifloor(3.9)                  // 3
```
