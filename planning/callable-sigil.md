---
status: DONE
priority: (MUST, SOONER, LOW, BROAD)
---

# Swap `@` / `#` — callable sigil and import sigil

## Motivation

Callables and grouped expressions are currently ambiguous — both start with `(`.
The parser resolves this with a lookahead heuristic (`looks_like_callable()`: scan to matching `)`, check if `{` follows). This is fragile and prevents `(expr) { hash }` from ever being written as two adjacent statements.

A callable sigil eliminates the ambiguity with zero lookahead. `@` is the natural choice; `#` is the natural choice for import.

## The swap

| Today | After |
|-------|-------|
| `@ "lib/stdio.vo"` | `# "lib/stdio.vo"` |
| `(x, y) { x + y }` | `@(x, y) { x + y }` |

- **`#`** — universally read as preprocessor / include / meta (`#include`, `#import`). Zero ambiguity with any future operator.
- **`@`** — already carries "at / address / callable" semantics (Python decorator + matrix multiply, Ruby instance vars). `@(x) { body }` reads naturally as a callable literal.

## Implementation

*Step 1 — lexer*: `@` already emits `TokenType::At`. No token type change needed — the parser just gains a new interpretation for `At`.

*Step 2 — parser `parse_primary()`*:
- Replace the `check(TT::At)` import branch: `#` (`TT::Hash`, new token) triggers `parse_import()`
- Add `check(TT::At)` branch *before* the `LParen` check: consume `@`, then call `parse_callable()` directly — no lookahead needed
- Remove `looks_like_callable()` and its call site entirely

*Step 3 — parser `parse_stmt()`*:
- Import now triggered by `TT::Hash` instead of `TT::At`

*Step 4 — lexer*:
- Add `#` -> `TT::Hash` token (currently `#` hits `default: error(...)`)

*Step 5 — migrate all `.vo` files*:
- Global find-and-replace `@ "` -> `# "` across `interp/lib/`, `interp/tests/`, `interp/examples/`, `interp/game/`
- Mechanical; no logic changes

*Step 6 — extend test coverage*:
- Add cases to `tests/test_callable_syntax.vo` that explicitly exercise `@(x) { }` in positions that previously relied on `looks_like_callable()`:
  - Zero-param callable: `@() { 42 }`
  - Callable immediately followed by a hash literal (the previously ambiguous case): `f = @(x) { x }  data = { key = 1 }`
  - Callable as a hash member value
  - Callable passed as an argument

## Result

```vo
# "lib/stdio.vo"         // import — reads like #include
# "lib/loops.vo"

add = @(a : int, b : int) { a + b }   // callable — unambiguous
f = @(x) { x * 2 }
result = f(21)
```

Grammar becomes unambiguous with one token of lookahead. `looks_like_callable()` deleted. `#` is safe from future operator conflicts; `@` has no plausible future use as an arithmetic operator.

**Breaking change**: all existing `.vo` source files need the mechanical `@ "` -> `# "` substitution.
