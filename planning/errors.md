---
status: PENDING
priority: (MUST, SOONER, LOW, BROAD)
---

# Error reporting — line numbers

## Current state

Errors are reported without source location:

```
RuntimeError: Member access on non-hash value 'foo'
ParseError: expected '{' but got 'bar'
```

Tokens already carry `line` and `col` (`token.hpp:56`). The lexer tracks them internally. The parser has a `last_line_` field (`parser.hpp:23`). But AST nodes (`nodes.hpp`) carry no location, so by runtime the position is gone.

## What to fix

**Parse errors** — include filename, approximate line, and column in every `throw ParseError(...)`. The parser always has the current token with `line`/`col` already set. Line is approximate for multi-byte UTF-8 source (counts newline bytes, not codepoints — exact fix is Phase 6 of `unicode_plan.md`). Column is byte-based similarly.

**Runtime errors** — add a `line` field to AST nodes so the interpreter can report where an expression came from. The parser already has `last_line_` as a seed; it just needs to be stamped onto nodes at parse time.

## Proposed change

1. Pass the source filename into `Parser` and store it.
2. Add `int line = 0;` to the `Expr` and `Stmt` base structs in `nodes.hpp`.
3. Stamp `line` (and optionally `col`) from the current token in the parser wherever nodes are constructed.
4. Include `filename:line:col` in `ParseError` messages at every throw site.
5. Pass `line` through to `RuntimeError` at throw sites in `interpreter.cpp`.
6. Output format:
   ```
   foo.vo:4:12: ParseError: expected '{' but got 'bar'
   foo.vo:2:1:  RuntimeError: Member access on non-hash value 'foo'
   ```

Column for runtime errors is optional — line alone is usually enough. Column is correct for parse errors (current token).

