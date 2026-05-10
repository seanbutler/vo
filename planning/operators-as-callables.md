---
status: PENDING
priority: (MUST, LATER, HIGH, BROAD)
depends: callable-sigil.md, infix-syntax.md
---

# Operators as first-class callables

## Vision

Arithmetic operators `+` `-` `*` `/` `%` become pre-bound callable identifiers. Infix syntax is purely a parser rewrite — `a + b` desugars to `+(a, b)` at parse time. Users can shadow, redefine, or delegate operators like any other callable.

```vo
// built-in pre-bindings (in stdlib or interpreter bootstrap)
+ = @(a : int  b : int) { ... }   // native add
* = @(a : int  b : int) { ... }

// user override — redefine + for a vector type
Vec = {
    x : int = 0
    y : int = 0
    + = @(other) { Vec(self.x + other.x  self.y + other.y) }
}
```

## Precedence decision

The current parser encodes precedence structurally (multiplicative ladder sits above additive). If operators become plain callables this structure must change. Two options:

1. **No precedence — explicit parentheses required** *(preferred)*
   - `3 + 4 * 2` becomes a parse error or left-to-right: must write `3 + (4 * 2)`
   - Consistent with VO's philosophy — no hidden rules, programmer states intent explicitly
   - Simplifies the parser significantly (flat binary expression level)

2. **Precedence metadata on callables**
   - Each operator callable carries a `_prec` slot (e.g. `*.prec = 7  +._prec = 5`)
   - Parser reads this at parse time to order rewrites
   - Complex; mixes runtime values into parse-time decisions

**Preferred: adopt no precedence, require explicit parentheses.**

## Motivating example — `!` (logical NOT)

`!` is currently a C++ primitive in the interpreter (`interpreter.cpp:333`). Once prefix operator dispatch is in place, it becomes a pre-bound VO callable:

```vo
! = @(a) { ? a { 0 } { 1 } }
```

The C++ case is deleted. `! x` desugars to `!(x)` at parse time, which looks up `!` in the environment like any other name. `logic.not` can then simply alias it:

```vo
logic = {
    not = !       // ! is just a callable value
    ...
}
```

This is the concrete first target for the operator dispatch mechanism — a clean, testable removal of one hardcoded interpreter primitive.

## Steps

**Step 1 — infix rewrite for named functions (prerequisite)**

Implement backtick infix syntax first (see `infix-syntax.md`). This establishes the parse-time rewrite infrastructure that operator desugaring will reuse.

**Step 2 — make operator glyphs valid identifier starts in the lexer**

Currently `+` `-` `*` `/` `%` are single-char tokens consumed before the identifier path. Extend the lexer so that a bare `+` (not followed by a digit or another operator) is tokenised as an `Identifier` with lexeme `"+"`. Requires careful ordering in `tokenize()` to avoid breaking numeric literals and existing operator tokens.

**Step 3 — flatten the parser precedence ladder**

Replace the `parse_additive` / `parse_multiplicative` / `parse_comparison` chain with a single `parse_binary()` level. All infix operator tokens rewrite to calls at the same precedence — left to right. Mixed expressions without parentheses are a parse error (forces explicitness).

**Step 4 — bootstrap operator bindings**

Pre-bind `+` `-` `*` `/` `%` in the interpreter's `register_builtins()` (or a `lib/operators.vo`) as native callables wrapping the existing C++ arithmetic.

**Step 5 — comparison operators**

Same treatment for `==` `!=` `<` `<=` `>` `>=` — each becomes a pre-bound callable. This enables user-defined equality and ordering for custom hash types.

**Step 6 — remove operator tokens from the token set**

Once all operator glyphs are identifiers, `TT::Plus`, `TT::Minus`, `TT::Star`, `TT::Slash`, `TT::Percent` are removed from the token type enum. Comparison token types follow once Step 5 is complete.

## Result

```vo
# "lib/operators.vo"    // provides + - * / % == != < <= > >=

x = (3 + (4 * 2))      // explicit parentheses required — no precedence
y = 3 + 4 * 2          // parse error: ambiguous without parentheses

Vec = {
    x : int = 0
    y : int = 0
    + = @(other) { Vec(self.x + other.x  self.y + other.y) }
}
v1 = Vec(1  2)
v2 = Vec(3  4)
v3 = v1 + v2            // desugars to +(v1, v2) -> Vec.+ lookup via self
```

**Breaking change**: all existing expressions using `+` `-` `*` `/` `%` without explicit parentheses around mixed operators will need parentheses added.
