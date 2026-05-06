---
status: PENDING
priority: (COULD, SOONER, SMALL, BROAD)
---

# User-defined infix call syntax

Allow any two-argument callable to be called in infix position using backtick syntax.
`` a `f` b `` desugars to `f(a, b)` at parse time — no new runtime machinery.

## Motivation

- `logic.and`, `logic.or`, `logic.xor` etc. become readable without a module prefix workaround
- User-defined operators (e.g. `` vec `dot` other ``, `` list `append` item ``) read naturally
- Stays true to VO's no-reserved-words philosophy — backtick is a syntactic escape, not a keyword

## Syntax

```
a `and` b          // desugars to: and(a, b)
a `logic.and` b    // dotpath lookup, desugars to: logic.and(a, b)
x `dot` y `add` z  // left-associative: add(dot(x, y), z)
```

## Implementation — parser only

After parsing any primary expression, check if next token is a backtick-delimited identifier.

**Step 1 — lexer**
- Recognise `` ` `` as the start of a backtick token
- Read until closing `` ` ``, emit `TOKEN_BACKTICK_IDENT` carrying the name

**Step 2 — parser (`parse_expr` / `parse_binary`)**
- In the binary-expression loop, after the primary, check for `TOKEN_BACKTICK_IDENT`
- Consume it, parse next primary, emit `CallExpr`
- Left-associative; same precedence tier as function calls
