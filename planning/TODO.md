# TODO

Each item has a priority tag: `(IMPORTANCE, URGENCY, COST, BENEFIT)`

| Dimension                        | Simple Choice     |    |  
|---                               |---                |--- |
| **IMPORTANCE**:                  | MUST     | COULD  |
| **URGENCY**:                     | SOONER   | LATER  |
| **COST** (effort × complexity) : | LOW      | HIGH   |
| **BENEFIT**:                     | LIMITED  | BROAD  | 

---

## Dependency map

Hard dependencies ( -> means "must be done before" ):

**Language design chain**

[infix-syntax](infix-syntax) -> [operators-as-callables](operators-as-callables) -> [unified-dispatch](unified-dispatch)

[callable-sigil](callable-sigil) -> [hash-arrays](hash-arrays) (.(expr) syntax confirmed; no bracket syntax needed)


**Publication chain**

[visitor-dispatch](visitor-dispatch) ->  [cache-aware-layout](cache-aware-layout) -> Paper 3 [bytecode-vm](bytecode-vm) -> Paper 4

[visitor-dispatch](visitor-dispatch) -> [ast-api](ast-api) -> [homoiconicity](homoiconicity)
[unified-dispatch](unified-dispatch) + [homoiconicity](homoiconicity) -> Paper 1
Paper 1 || Paper 3/4 -> Paper 2  (speculative)



**Localisation**

[unicode_plan](unicode_plan) phases 2–6 -> [localisation](localisation) Layer 1

**Tooling**

[visitor-dispatch](visitor-dispatch)
[ast-visualisation](ast-visualisation)


Items with no upstream dependencies (safe to start anytime):

- [drop-commas](drop-commas.md)
- [lazy-boolean](lazy-boolean)
- [immutability](immutability)
- [stdio-design](stdio-design.md)
- [sdl3-binding](sdl3-binding.md)
- [ffi-builtins](ffi-builtins)
- [runtime-controls](runtime-controls)
- [tail-call](tail-call)
- [profiling](profiling)
- [visitor-dispatch](visitor-dispatch)
- [code-size](code-size)
- [lib-management](lib-management)

---

## Pending

### Language design

| Item | Priority | File |
|------|----------|------|
| Hash arrays — insertion order, variant keys, `.(expr)` integer access | MUST, SOONER, MEDIUM, BROAD | [hash-arrays.md](hash-arrays.md) |
| Drop comma from parameter lists | MUST, SOONER, LOW, BROAD | [drop-commas.md](drop-commas.md) |
| Lazy boolean operators `&` and `\|` | MUST, SOONER, SMALL, BROAD | [lazy-boolean.md](lazy-boolean.md) |
| User-defined infix call syntax (backtick) | COULD, SOONER, SMALL, BROAD | [infix-syntax.md](infix-syntax.md) |
| Localisation — three-layer architecture | MUST, SOONER, HIGH, BROAD | [localisation.md](localisation.md) |
| Operators as first-class callables | MUST, LATER, HIGH, BROAD | [operators-as-callables.md](operators-as-callables.md) |
| Unified dot / infix / UFCS dispatch | MUST, LATER, HIGH, BROAD | [unified-dispatch.md](unified-dispatch.md) |
| AST construction API — node builders, env manipulation, eval | MUST, LATER, HIGH, BROAD | [ast-api.md](ast-api.md) |
| Homoiconicity — `parse` and `eval` (builds on AST API) | COULD, LATER, HIGH, BROAD | [homoiconicity.md](homoiconicity.md) |
| Enforce immutability at runtime | MUST, LATER, LOW, BROAD | [immutability.md](immutability.md) |
| Output / stdio design | MUST, LATER | [stdio-design.md](stdio-design.md) |
| Default parameter values | COULD, LATER, LOW, BROAD | [default-params.md](default-params.md) |

### Systems / FFI

| Item | Priority | File |
|------|----------|------|
| Library management — path resolution, import guards, search path | MUST, SOONER, MEDIUM, BROAD | [lib-management.md](lib-management.md) |
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
| Error reporting — filename, line, column in parse/runtime errors | MUST, SOONER, LOW, BROAD | [errors.md](errors.md) |

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

## [Callable sigil + import sigil swap](callable-sigil.md)
- (MUST, SOONER, LOW, BROAD)

`@` is now the callable literal sigil; `#` is now the import sigil.

| Before | After |
|--------|-------|
| `(params) { body }` | `@(params) { body }` |
| `@ "lib/foo.vo"` | `# "lib/foo.vo"` |
| `>> (k, v) { }` | `>> @(k, v) { }` |

**C++ changes:**
- `token.hpp` — added `Hash` token type; `At` retained for callable sigil
- `lexer.cpp` — `#` -> `Hash`, `@` -> `At`
- `parser.cpp` — import trigger changed from `At` to `Hash`; `parse_primary()` checks for `At` before `parse_callable()`; `>>` RHS changed to `parse_expr()`; `looks_like_callable()` heuristic removed entirely
- `parser.hpp` — `looks_like_callable()` declaration removed

**Migration:** all 40+ `.vo` files updated via three-pass script (sed for imports, sed for `>>` callbacks, Python regex with `(?<!\w)(?<!@)\(([^\(\)\{\}]*)\)(\s*\{)` for remaining callable literals).

All 17 tests pass including emoji-param unicode test.

## Unicode identifiers — Phase 1
- Lexer accepts any UTF-8 byte sequence as an identifier (high bytes > 127 treated as identifier characters)
- Two-line change in `lexer.cpp` — `read_word()` continuation and `tokenize()` start condition
- Test coverage in `interp/tests/test_unicode.vo` — emoji, extended-ASCII, mixed unicode+ASCII
- Phases 2–6 pending in `unicode_plan.md`

## Boolean logic operators — `lib/logic.vo`
- (MUST, SOONER, SMALL, BROAD)

All boolean operators derived from `?` conditionals — no new C primitives required.
`logic.vo` exposes a `logic` hash; import with `# "lib/logic.vo"`.

| Callable | Expression |
|---|---|
| `logic.not(a)` | `? a { 0 } { 1 }` |
| `logic.and(a,b)` | `? a { b } { 0 }` |
| `logic.or(a,b)` | `? a { 1 } { b }` |
| `logic.xor(a,b)` | `? a { ? b { 0 } { 1 } } { ? b { 1 } { 0 } }` |
| `logic.nand(a,b)` | `? a { ? b { 0 } { 1 } } { 1 }` |
| `logic.nor(a,b)` | `? a { 0 } { ? b { 0 } { 1 } }` |
| `logic.xnor(a,b)` | `? a { b } { ? b { 0 } { 1 } }` |

Full truth-table test coverage in `tests/test_logic.vo`.


## Loop syntax

- `~{ body }` — infinite loop block; repeats until `\` is executed
- `\` — break/escape; lexically scoped to enclosing `~{ }`, parse-time enforced
- `!` — logical NOT (prefix unary); `!x`, `!!(x)` etc.
- `\` inside a callable defined inside `~{ }` is a parse error (loop_depth_ reset on callable entry)
- `BreakSignal` C++ exception thrown by `\`, caught at `~{ }` boundary (mirrors `ReturnSignal`)

```
~{ ? !cond { \ }  body }     // while
~{ body  ? !cond { \ } }     // do-while
~{ body }                    // infinite
```


## OOP — inheritance and private slots

- **`_` delegation (interpreter)**
  - `HashInstance::get` walks the `_` slot as a prototype chain before throwing
  - Applies recursively — multi-level inheritance works automatically
  - Assignment (`set`) is always local — never writes through the chain
  - `()` constructor found through `_` chain at call time — no copying needed

- **Private slots (interpreter)**
  - Any slot whose name begins with `_` is skipped by `>>` and by `to_display_string`
  - Directly accessible by name: `obj._thing` works as normal
  - `merge`, `clone`, and all stdlib functions built on `>>` naturally exclude private slots
  - The `_` delegation slot is itself private under this rule — no special case needed

- **`subtype` (`lib/stdlib.vo`)**
  - `subtype(parent, overrides)` — merges public slots, sets `_ = parent`, returns child hash
  - Constructor, methods, and data slots all flow through delegation or direct copy
  - Override any slot by including it in `overrides`; method override replaces the slot, parent unaffected

```
Animal = {
    sound : string = "..."
    speak  = () { self.sound }
    () = (s : string) { self.sound := s }
}
Dog    = subtype(Animal, { sound : string = "Woof" })
Poodle = subtype(Dog,    { size  : string = "small" })
p = Poodle("Fifi")    // constructor found via _ _ chain
p.speak()             // method inherited from Animal
```


## null alias for `{}`
- (COULD, SOONER, SMALL, SMALL)

- `{}` is already the language's null/empty sentinel
- `null = {}` added to `lib/stdlib.vo`
- FFI pointer dispatcher treats empty hash as `nullptr`


## Bare block `{ }` as zero-arg callable
- (COULD, LATER, SMALL, SMALL)

A `{ body }` in expression position with no leading param list is sugar for `() { body }`. Makes `func_name = { code }` a callable, invoked as `func_name()`.


## Terminal graphics via ANSI escape codes
- (COULD, SOONER, SMALL, LARGE)

Cursor-addressed terminal output and non-blocking keyboard input. No curses/ncurses dependency.

C shim: `interp/term/voterm.c` -> `libvoterm.so`
VO descriptor: `interp/lib/term.vo` -> `term` hash

| Call | Effect |
|------|--------|
| `term.clear()` | clear screen, cursor to 0,0 |
| `term.goto(x, y)` | move cursor to column x, row y |
| `term.color(n)` | set foreground colour 0–7 |
| `term.bg(n)` | set background colour 0–7 |
| `term.reset()` | reset fg+bg to terminal default |
| `term.hide_cursor()` | hide cursor for clean animation |
| `term.show_cursor()` | restore cursor on exit |
| `term.raw_mode()` | enable non-blocking single-char input |
| `term.restore_mode()` | restore normal terminal input on exit |
| `term.getch()` | returns keycode int or -1 if no key pressed |

Colour constants: `term.BLACK=0  term.RED=1  term.GREEN=2  term.YELLOW=3  term.BLUE=4  term.MAGENTA=5  term.CYAN=6  term.WHITE=7`

---

## Related

- [PUBLISH.md](PUBLISH.md) — paper candidates and publication strategy
- [publication-roadmap.md](publication-roadmap.md) — dependency order for the four papers
