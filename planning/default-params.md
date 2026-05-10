---
status: PENDING
priority: (COULD, LATER, LOW, BROAD)
---

# Default parameter values

## Syntax

Extend the existing `name : type` parameter form with an optional default expression:

```vo
greet = @(name : string = "world") { "hello " + name }

greet()          // "hello world"
greet("sean")    // "hello sean"
```

Without a type annotation, `=` in param position is unambiguous (it cannot be a
binding statement there):

```vo
add = @(x  y = 0) { x + y }

add(3)     // 3
add(3  2)  // 5
```

## Rules

- Defaults are evaluated at **call time**, not definition time (avoids mutable default traps).
- All defaulted params must trail non-defaulted params.
- Passing `{}` (null) for a defaulted param does **not** trigger the default — only omitting the argument does.

## Implementation

`Param` in `nodes.hpp:25` gains an optional default expression:

```cpp
struct Param {
    std::string              name;
    std::optional<std::string> type_ann;
    ExprPtr                  default_expr;  // nullptr = required
};
```

In `interpreter.cpp`, at the call site, when `args.size() < fn.params.size()`, fill
remaining params from their `default_expr` (evaluated in the caller's env). Arity
mismatch error only fires if a required (no default) param is unsatisfied.

## Interaction with drop-commas

Separator between params is already whitespace (pending `drop-commas.md`). The `=`
introduces no ambiguity because `=` cannot appear in the param name or type annotation.
