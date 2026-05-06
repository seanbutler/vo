---
status: PENDING
priority: (MUST, SOONER, LOW, BROAD)
---

# Drop comma from parameter lists

Parameter lists currently require commas between parameters:

```vo
add = @(a : int, b : int) { a + b }
```

Commas are not needed — parameters are always `name` or `name : type`, both starting with an identifier, so whitespace separation is unambiguous. Hash members already use whitespace separation with no comma; dropping commas from parameter lists makes the language fully consistent:

```vo
add = @(a : int  b : int) { a + b }
```

Call arguments retain commas for now — `f(a -b)` is ambiguous without them (unary minus vs binary minus).

**Implementation — parser only**

- In `parse_param_list()`, replace `while (match(TT::Comma))` with a loop that continues as long as the next token is an identifier (i.e. the start of a new parameter) and not `TT::RParen`
- No lexer changes; no AST changes; no runtime changes

**Migration**

- Remove all commas from parameter lists in `interp/lib/`, `interp/tests/`, `interp/examples/`, `interp/game/`
- Mechanical find-and-replace; existing tests confirm correctness after

**Breaking change**: all callables with multiple parameters need commas removed from their parameter lists.
