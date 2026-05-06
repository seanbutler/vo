# TODO

Each item has a priority tag: `(IMPORTANCE, URGENCY, COST, BENEFIT)`

- **IMPORTANCE**: MUST / SHOULD / COULD
- **URGENCY**: SOONER / LATER
- **COST**: LOW / MEDIUM / HIGH (effort × complexity)
- **BENEFIT**: SMALL / BROAD / LARGE / LIMITED

---

## Dependency map

Hard dependencies ( -> means "must be done before" ):


**Language design chain**

[infix-syntax](infix-syntax) -> [operators-as-callables](operators-as-callables) -> [unified-dispatch](unified-dispatch)

[callable-sigil](callable-sigil)



**Publication chain**


visitor-dispatch ⟶  cache-aware-layout          -> Paper 3
                                                -> Paper 4  (also needs bytecode VM)
unified-dispatch + homoiconicity                -> Paper 1
Paper 1 || Paper 3/4                            -> Paper 2  (speculative)



**Localisation**

[unicode_plan](unicode_plan) phases 2–6 -> [localisation](localisation) Layer 1

**Tooling**

[visitor-dispatch](visitor-dispatch)
[ast-visualisation](ast-visualisation)


Items with no upstream dependencies (safe to start anytime):
`drop-commas`, `lazy-boolean`, `immutability`, `stdio-design`, `sdl3-binding`, `ffi-builtins`, `runtime-controls`, `tail-call`, `profiling`, `visitor-dispatch`, `code-size`

---

## Pending

### Language design

| Item | Priority | File |
|------|----------|------|
| Swap `@`/`#` — callable sigil + import sigil | MUST, SOONER, LOW, BROAD | [callable-sigil.md](callable-sigil.md) |
| Drop comma from parameter lists | MUST, SOONER, LOW, BROAD | [drop-commas.md](drop-commas.md) |
| Lazy boolean operators `&` and `\|` | MUST, SOONER, SMALL, BROAD | [lazy-boolean.md](lazy-boolean.md) |
| User-defined infix call syntax (backtick) | COULD, SOONER, SMALL, BROAD | [infix-syntax.md](infix-syntax.md) |
| Localisation — three-layer architecture | MUST, SOONER, HIGH, BROAD | [localisation.md](localisation.md) |
| Operators as first-class callables | MUST, LATER, HIGH, BROAD | [operators-as-callables.md](operators-as-callables.md) |
| Unified dot / infix / UFCS dispatch | MUST, LATER, HIGH, BROAD | [unified-dispatch.md](unified-dispatch.md) |
| Homoiconicity — `parse` and `eval` builtins | COULD, LATER, HIGH, BROAD | [homoiconicity.md](homoiconicity.md) |
| Enforce immutability at runtime | MUST, LATER, LOW, BROAD | [immutability.md](immutability.md) |
| Output / stdio design | MUST, LATER | [stdio-design.md](stdio-design.md) |

### Systems / FFI

| Item | Priority | File |
|------|----------|------|
| SDL3 binding via C shim | MUST, SOONER, LOW, HIGH | [sdl3-binding.md](sdl3-binding.md) |
| Replace hardcoded NativeCallable builtins with FFI | COULD, SOONER, MEDIUM, BROAD | [ffi-builtins.md](ffi-builtins.md) |
| Interpreter runtime controls | COULD, SOONER, SMALL, BROAD | [runtime-controls.md](runtime-controls.md) |

### Performance

| Item | Priority | File |
|------|----------|------|
| Cache-aware AST layout | COULD, LATER, MEDIUM, LARGE | [cache-aware-layout.md](cache-aware-layout.md) |
| Tail-call optimisation | COULD, LATER, HIGH, BROAD | [tail-call.md](tail-call.md) |

### Tooling

| Item | Priority | File |
|------|----------|------|
| Profiling report (`--profile`) | COULD, SOONER, MEDIUM, LARGE | [profiling.md](profiling.md) |
| AST visualisation via Graphviz (`--ast`) | COULD, SOONER, MEDIUM, LARGE | [ast-visualisation.md](ast-visualisation.md) |
| Publication roadmap | MUST, SOONER, LOW, BROAD | [publication-roadmap.md](publication-roadmap.md) |

### Engineering / internals

| Item | Priority | File |
|------|----------|------|
| Visitor-style dispatch refactoring | SHOULD, LATER, HIGH, LIMITED | [visitor-dispatch.md](visitor-dispatch.md) |
| Code size & complexity | COULD, LATER, MEDIUM, SMALL | [code-size.md](code-size.md) |

### Unicode safety (phases 2–6)

See [unicode_plan.md](unicode_plan.md) for the full plan.

| Item | Phase |
|------|-------|
| Reject bidirectional control characters (Trojan Source) | Phase 2 |
| Reject zero-width characters in identifiers | Phase 3 |
| Document homoglyph risk | Phase 4 |
| NFC normalisation at lex time | Phase 5 |
| Fix column counter to count codepoints | Phase 6 |

---

## Done

See [done.md](done.md) for details.

- Unicode identifiers — Phase 1 (UTF-8 byte pass-through in lexer)
- Boolean logic operators — `lib/logic.vo`
- Loop syntax — `~{ }`, `\`, `!`
- OOP — `_` delegation, private slots, `subtype`
- `null` alias for `{}`
- Bare block `{ }` as zero-arg callable
- Terminal graphics via ANSI escape codes — `lib/term.vo`

---

## Related

- [PUBLISH.md](PUBLISH.md) — paper candidates and publication strategy
- [publication-roadmap.md](publication-roadmap.md) — dependency order for the four papers
