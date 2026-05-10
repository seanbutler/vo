---
status: PENDING
priority: (MUST, LATER, HIGH, BROAD)
depends: operators-as-callables.md
---

# Unified dot / infix / prefix — one dispatch rule

## Insight

Dot notation and infix operators are the same transformation — first argument moves left of the function name:

```
f(a, b)   ->   a.f(b)    // dot notation
add(a, b)   ->   a + b     // infix operator
```

Both are syntactic sugar for the same underlying call. Unifying them under one dispatch rule gives:

1. **Look up the function name as a slot on the first argument** (prototype chain) — operator overloading
2. **Fall through to global scope** — UFCS (Uniform Function Call Syntax)

```vo
a + b          // sugar for a.+(b)
a.+(b)         // explicit dot form of the same call
+(a, b)        // pure prefix — global lookup only, no receiver

a.f(b)         // looks up f on a first, falls through to global f(a, b)
f(a, b)        // pure prefix equivalent
```

## Operator overloading

Falls out for free — add a `+` slot to any hash and it intercepts `a + b` for that type:

```vo
Vec = {
    x : int = 0
    y : int = 0
    + = @(other) { Vec(self.x + other.x  self.y + other.y) }
}
v1 = Vec(1  2)
v2 = Vec(3  4)
v3 = v1 + v2    // a.+(b) -> looks up + on v1 -> Vec.+
```

## UFCS

Dot and prefix are interchangeable when the name is not a member:

```vo
double = @(x : int) { x * 2 }
5.double()     // looks up double on 5 (int), not found -> falls through to global -> double(5)
```

## The single dispatch rule

`.` means: look up the name starting from the left operand's prototype chain, then global scope; pass the left operand as the first argument.

Infix operator syntax is `.` with the function name written between operands instead of after the dot. They are one rule, not two.

## Consequences

- Dot notation, infix operators, and UFCS unify into one mechanism
- Operator overloading requires no new language feature — just a `+` slot on a hash
- The symbol table shrinks — `.` and infix dispatch are the same rule
- Prefix `f(a, b)` bypasses receiver lookup entirely — explicit global call
- Complements operators-as-callables (see `operators-as-callables.md`): once `+` is a callable identifier, `a.+(b)` is already valid syntax

## Implementation

Depends on: operators as first-class callables, flat binary expression parser (no hardcoded precedence ladder).

*Step 1*: extend member lookup in `parse_postfix()` — after `.name`, if followed by `(`, pass left operand as implicit first argument alongside explicit args.

*Step 2*: extend infix operator rewrite — `a op b` rewrites to `a.op(b)` rather than `op(a, b)`; receiver lookup attempted first, global fallback second.

*Step 3*: update `HashInstance::get` / interpreter call path to support the fallthrough from receiver to global scope.

*Step 4*: update `self` binding — `self` is bound to the receiver (left operand) when lookup succeeds on the prototype chain; not bound on global fallthrough.
