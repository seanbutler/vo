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

## Stretch goal — library-defined syntax rewrites

The broader ambition is that all syntax extensions live in `.vo` files, not in the
interpreter. The token rewriting mechanism that handles infix operators should be
expressive enough to cover three forms:

| Form | Example | Desugars to |
|------|---------|-------------|
| Binary infix | `a + b` | `+(a, b)` |
| Prefix unary | `! x` | `not(x)` |
| Bracketed postfix | `a[b]` | `a.(b)` |

The bracketed postfix case is the hard one — the rewriter needs to understand **matched
delimiter pairs** with an expression inside, not just single operator tokens.

With this in place, `[]` subscript syntax becomes a library feature:

```vo
# "lib/subscript.vo"   // registers: expr [ expr ] -> expr.(expr)

arr[99]                // valid — rewrites to arr.(99)
arr["name"]            // valid — rewrites to arr.("name")
```

Without that import, `[` is an unknown token. No hidden interpreter behaviour — the
programmer opts in explicitly and can inspect the rewrite rule in the source.

This is the same philosophy as `!` eventually delegating to a VO-defined `not` via the
prefix rewrite mechanism: the interpreter provides irreducible primitives only;
everything else is expressible within the language itself.
