---
status: PENDING
priority: (MUST, SOONER, SMALL, BROAD)
---

# Lazy boolean operators `&` and `|`

- Add `&` (logical AND) and `|` (logical OR) as proper infix operators
- `a & b` desugars to `? a { b } { 0 }` — short-circuits, no call-frame overhead
- `a | b` desugars to `? a { 1 } { b }` — short-circuits
- Precedence: `|` below `&`, both below `!`, above comparison
