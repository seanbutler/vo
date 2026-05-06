# Done

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

C shim: `interp/term/voterm.c` → `libvoterm.so`
VO descriptor: `interp/lib/term.vo` → `term` hash

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
