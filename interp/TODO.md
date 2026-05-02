# TODO

## Index

- [Note: How to read this TODO...](#note-how-to-read-this-todo)
- [Pending](#pending)
- [Done](#done)

## Note: How to read this TODO...

Each todo item has a parenthesis in its title with the following form:
(IMPORTANCE, URGENCY, COST, BENEFIT)

e.g. (COULD, SOONER, SMALL, LARGE)

- MUST, COULD (from MOSCOW)
- SOONER, LATER simplest possible time priority
- LOW, HIGH  cost
- SMALL, LARGE benefit

This gives us a shorthand to record opinions about this feature useful in ordering and prioriting work.
- **important:** because it matches our goals
- **urgency:** because it has beneficial side effects for the project so has to be done before or other external priority from paper or engineering deadlines
- **cost:** the amount of engineering effort required, LARGE could mean big or complex.
- **benefit:** are the outcomes extensive from this todo item? either its a big feature, or popular feature or maybe almost nobody will use it


## PENDING

### SDL3 binding via C shim
- (MUST, SOONER, LOW, HIGH)
- PENDING

Open a window, run a game loop, draw, handle input — currently would prefer to be all from VO.
Lets not return c structs if possible. Perhaps we can translate some structs to vo tables later?
Perhaps we limit the lib and keep some windows and textures behind a wall and add getters/setters etc

**Step 1 — FFI extensions (interpreter.cpp)**
- Add `"pointer"` param/return type — stored as `int64_t`, cast to/from `void*`
- Add `"void"` return type — returns `Value::nil()`
- Extend `bind_foreign_function()` dispatch to handle `pointer` in any parameter position
- Empty hash `{}` (or `null` once aliased) passed as a `"pointer"` argument maps to `nullptr`

**Step 2 — C shim (`interp/sdl/vosdl.c`, compiled to `libvosdl.so`)**

Structs fall into two categories — the shim handles both transparently:

*Opaque handles* (`SDL_Window*`, `SDL_Renderer*`, `SDL_Texture*`) — heap-managed by SDL.
Shim provides thin create/destroy pairs; handles cross the boundary as `int64_t` pointers:
```c
void* vo_create_window(const char* t, int w, int h) {
    SDL_Init(SDL_INIT_VIDEO);
    return SDL_CreateWindow(t, w, h, 0);
}
void vo_destroy_window(void* win) { SDL_DestroyWindow(win); }
```

*Value structs* (`SDL_FRect`, `SDL_Color`, `SDL_Point`) — stack-allocated, no teardown.
Shim takes individual scalars, builds the struct internally, calls SDL, discards it:
```c
void vo_fill_rect(void* ren, int x, int y, int w, int h) {
    SDL_FRect r = { x, y, w, h };
    SDL_RenderFillRect(ren, &r);
}
```

VO side uses a hash as the struct; wrapper callables unpack fields at the call site:
```
rect = { x:int=0  y:int=0  w:int=0  h:int=0
         ()=(px:int,py:int,pw:int,ph:int){ self.x:=px self.y:=py self.w:=pw self.h:=ph } }

fill_rect = (ren, r) { SDL.fill_rect(ren, r.x, r.y, r.w, r.h) }
```

Full shim API:
- `vo_create_window(title, w, h)` → pointer — SDL_Init + SDL_CreateWindow
- `vo_create_renderer(win)` → pointer — SDL_CreateRenderer(win, NULL)
- `vo_destroy_window(win)`, `vo_destroy_renderer(ren)` — cleanup
- `vo_pump()` — drains SDL_Event queue, updates internal state
- `vo_quit()` — 1 if quit event received
- `vo_key(scancode)` — 1 if key currently held
- `vo_mouse_x()`, `vo_mouse_y()` — current cursor position
- `vo_set_draw_color(ren, r, g, b, a)` — set render colour
- `vo_clear(ren)`, `vo_present(ren)` — frame rendering
- `vo_fill_rect(ren, x, y, w, h)` — draw filled rectangle
- `vo_delay(ms)` — frame timing

**Step 3 — VO descriptor (`interp/lib/vosdl.vo`)**
- Descriptor hash + `bind_lib` call
- Exposes `SDL` hash with all shim functions bound

**Result — minimal VO game loop:**
```
@ "lib/vosdl.vo"

win = SDL.create_window("hello", 800, 600, null)
ren = SDL.create_renderer(win, null)
~{
    SDL.pump()
    ? SDL.quit()    { \ }
    ? SDL.key(41)   { \ }    // escape
    SDL.draw_color(ren, 0, 0, 0, 255)
    SDL.clear(ren)
    SDL.present(ren)
    SDL.delay(16)
}
SDL.destroy_renderer(ren)
SDL.destroy_window(win)
```

**Deferred — needs struct marshalling to add later:**
- `SDL_Texture` / sprite rendering
- `SDL_Rect` for clipping
- Full keyboard event stream (not just current state)
- Audio (`SDL_audio`)


### Lazy boolean operators `&` and `|`
- (MUST, SOONER, SMALL, BROAD)
- PENDING

- Add `&` (logical AND) and `|` (logical OR) as proper infix operators
- `a & b` desugars to `? a { b } { 0 }` — short-circuits, no call-frame overhead
- `a | b` desugars to `? a { 1 } { b }` — short-circuits
- Precedence: `|` below `&`, both below `!`, above comparison


### User-defined infix call syntax (COULD, SOONER, SMALL, BROAD)
- PENDING

Allow any two-argument callable to be called in infix position using backtick syntax.
`` a `f` b `` desugars to `f(a, b)` at parse time — no new runtime machinery.

**Motivation**
- `logic.and`, `logic.or`, `logic.xor` etc. become readable without a module prefix workaround
- User-defined operators (e.g. `` vec `dot` other ``, `` list `append` item ``) read naturally
- Stays true to VO's no-reserved-words philosophy — backtick is a syntactic escape, not a keyword

**Syntax**
```
a `and` b          // desugars to: and(a, b)
a `logic.and` b    // dotpath lookup, desugars to: logic.and(a, b)
x `dot` y `add` z  // left-associative: add(dot(x, y), z)
```

**Implementation — parser only**
- After parsing any primary expression, check if next token is a backtick-delimited identifier
- Lex the identifier between backticks as a normal name (dotpath allowed)
- Parse the right-hand side as the next primary, wrap as `CallExpr(ident, {lhs, rhs})`
- Left-associative; same precedence tier as function calls

**Step 1 — lexer**
- Recognise `` ` `` as the start of a backtick token
- Read until closing `` ` ``, emit `TOKEN_BACKTICK_IDENT` carrying the name

**Step 2 — parser (`parse_expr` / `parse_binary`)**
- In the binary-expression loop, after the primary, check for `TOKEN_BACKTICK_IDENT`
- Consume it, parse next primary, emit `CallExpr`


### Localisation — three-layer architecture (MUST, SOONER)
- PENDING

VO has no reserved words and symbol-only syntax, making it uniquely suited to full natural-language localisation. The goal is to separate three concerns cleanly:

```
1. Source (community symbols) → symbol table → canonical AST
2. Canonical AST → interpreter
3. Interpreter errors → error template table → community-language diagnostics
```

**Layer 1 — Configurable symbol table (lexer)**
- Move operator mappings out of the hardcoded lexer into an external table (JSON or similar)
- Communities remap any canonical symbol to any UTF-8 glyph (e.g. `?` → `если`, `:=` → `≔`)
- Loaded at startup; falls back to built-in defaults if absent
- Decide which symbols are fixed structural delimiters (braces, parens, comma) vs remappable

**Layer 2 — UTF-8 support (lexer + runtime)**
- Lexer currently treats source as raw bytes; multi-byte sequences may be mishandled
- Identifiers may contain UTF-8 characters (non-ASCII variable/function names)
- String literals preserve UTF-8 content correctly
- Line/column tracking counts Unicode codepoints, not bytes
- Operator glyphs in the symbol table may themselves be multi-byte
- Approach: decode source to codepoints before lexing, or UTF-8-aware character classification

**Layer 3 — Error message templates (runtime)**
- Externalise all `RuntimeError` and `ParseError` strings into a template table
- Templates must support:
  - Argument reordering (Arabic, Japanese, Turkish word order)
  - Gender agreement metadata
  - Plural forms (Russian 4 forms, Arabic 6 forms)
  - RTL/LTR direction hints
- Consider Mustache-style templates for community accessibility
- Library options: mstch, kainjow/mustache, or std::format (C++20)


### Enforce immutability at runtime (MUST, LATER)
- PENDING

- `DeclStmt` already carries `is_mutable` flag but the interpreter does not enforce it
- `set()` on an immutable binding should throw `RuntimeError`
- Requires tracking mutability in `Environment` alongside the value


### Output / stdio design (MUST, LATER)
- PENDING

Current `printf_s`/`printf_i` are problematic. Type already encoded in the name, format string adds no value. Three options to choose from:

  1. **Simple:** bind `puts(str)` for strings + a thin C wrapper `print_int(int)` — callers never see a format string
  2. **Better:** full varargs `printf(fmt, ...)` FFI support — useful when padding/alignment/precision matter
  3. **Current:** `printf_i("%d\n", n)` style — neither simple nor powerful. Fix this.


### Tail-call optimisation (COULD, LATER)
- PENDING

- The interpreter currently uses the C++ call stack for recursion
- Deep recursion (e.g. `range(1, 10000)`) will stack overflow
- Options: trampoline in `call_callable`, or explicit continuation passing
- Intentionally deferred — only needed if large recursion depths become a use case


### Profiling report (COULD, SOONER, MEDIUM, LARGE)
- PENDING

Goal: when `--profile` is passed, collect per-function timing and memory data and print a report on exit.

**Trigger**
- `--profile` flag on the `vo` binary; no effect on execution behaviour

**Metrics — per function**
- Call count
- Total wall time (inclusive — includes callees)
- Self wall time (exclusive — excludes callees)
- Peak memory allocated during the call (bytes)

**Metrics — totals**
- Total program wall time
- Total calls
- Peak memory across whole run

**Step 1 — timer and allocator hooks (interpreter.cpp)**
- Wrap `call_callable` with `std::chrono::steady_clock` timestamps
- Track a call stack depth counter to compute self-time (subtract child time from parent)
- Hook `Value` / `HashInstance` construction/destruction to count live allocations

**Step 2 — profile accumulator (`src/profiler.hpp/.cpp`)**
- Map from function name → `{call_count, total_ns, self_ns, peak_bytes}`
- Thread-safe if concurrency is ever added; for now a plain `std::unordered_map`
- Active only when `--profile` flag is set — zero overhead otherwise

**Step 3 — report printer**
- On program exit, sort entries by self time descending
- Print a table to stdout:
```
PROFILE REPORT
──────────────────────────────────────────────────────
 Function          Calls   Total ms   Self ms   Peak KB
──────────────────────────────────────────────────────
 loops.for          1200     48.2ms    12.1ms      4 KB
 draw_scene          600     36.1ms    36.1ms      2 KB
 ...
──────────────────────────────────────────────────────
 TOTAL                        48.2ms              8 KB
```

**Result — usage:**
```
./vo --profile mygame.vo
```


### AST Visualisation via Graphviz (COULD, SOONER, MEDIUM, LARGE)
- PENDING

Render the parsed AST as a Graphviz `.dot` graph, one per source file and one combined graph for the whole program including imports.

**Trigger**
- `--ast` flag passed to the `vo` binary — dumps AST after parse, before execution
- Automatic on each `@` import when `--ast` is active

**Output**
- Per-file: `<source_stem>.dot` (e.g. `main.vo` → `main.dot`)
- Combined: `program.dot` — all files merged into one digraph, subgraphs per file

**Step 1 — `--ast` flag (main.cpp)**
- Parse argv for `--ast` before running; pass a bool into `Interpreter` or a new `AstPrinter`
- Strip the flag from the file argument list so the interpreter still runs normally

**Step 2 — Graphviz emitter (`src/ast/dot_emitter.hpp/.cpp`)**
- Visitor over the AST (expressions + statements)
- Each node becomes a Graphviz node with a unique ID (e.g. pointer address or counter)
- Label: node type + key field (literal value, operator, identifier name)
- Edges: parent → child for each child slot
- Output: valid `.dot` file, one `digraph` per file, `subgraph cluster_<file>` in combined output

**Step 3 — Per-file emit on `@` import (interpreter.cpp)**
- After parsing an imported file, if `--ast` active, call the emitter and write `<stem>.dot`
- Append the file's nodes/edges into the combined graph accumulator

**Step 4 — Combined graph flush (main.cpp)**
- After the full program has loaded (all imports resolved), write `program.dot`

**Result — usage:**
```
./vo --ast mygame.vo
# produces: mygame.dot, stdlib.dot, vtkit.dot, ... program.dot
dot -Tpng program.dot -o program.png && xdg-open program.png
```


### Visitor-style dispatch refactoring (SHOULD, LATER, HIGH, LIMITED)
- PENDING

- Replace `dynamic_cast` chains in `Interpreter::eval` / `Interpreter::exec` with a proper visitor pattern
- Introduce AST visitor interfaces for expressions and statements
- Add `accept(...)` methods to all AST node types
- Migrate incrementally, keep behaviour parity with existing tests
- Remove dynamic-cast chains once all nodes dispatch through visitors
- Goal: clearer extension path and stronger compile-time coverage when adding AST nodes
- Intentionally deferred until feature set is more stable


### Code Size & Complexity (COULD, LATER, MEDIUM, SMALL)
- PENDING

Shorten Overall Code Length - Seems Excessive for a Small Language


## DONE

### Boolean logic operators — `lib/logic.vo`
- (MUST, SOONER, SMALL, BROAD)
- DONE

All boolean operators derived from `?` conditionals — no new C primitives required.
`logic.vo` exposes a `logic` hash; import with `@ "lib/logic.vo"`.

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


### Loop syntax
- DONE

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


### OOP — inheritance and private slots
- DONE

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

- **`subtype` (lib/stdlib.vo)**
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


### null alias for `{}`  (COULD, SOONER, SMALL, SMALL)
- DONE

- `{}` is already the language's null/empty sentinel — used wherever "nothing" is needed
- Add a stdlib binding so users can write `null` instead of `{}`
- Simplest implementation: one line in `lib/stdlib.vo` — `null = {}`
- No language changes required; `null` is just an identifier bound to the empty hash
- FFI pointer dispatcher should treat empty hash as `NULL` (i.e. `nullptr`) — relevant for SDL3 and any C library that takes optional pointer arguments


### Bare block `{ }` as zero-arg callable (COULD, LATER, SMALL, SMALL)
- DONE

A syntax change that potentially breaks backward compatability. A `{ body }` in expression position with no leading param list is sugar for `() { body }`. Makes `func_name = { code }` a callable, invoked as `func_name()`. Currently `{ }` is always parsed as a hash literal — parser needs to distinguish.


### Terminal graphics via ANSI escape codes (COULD, SOONER, SMALL, LARGE)
- DONE

Cursor-addressed terminal output and non-blocking keyboard input suitable for simple video games, snake, roguelikes, text UI. No curses/ncurses dependency.

**Step 1 — FFI extension (interpreter.cpp)**
- Add `"void"` return type — returns `Value::nil()`
- This is the only FFI change needed; no pointer type required

**Step 2 — C shim (`interp/term/voterm.c`, compiled to `libvoterm.so`)**
```c
void vo_clear()            // \033[2J\033[H — clear screen
void vo_goto(int x, int y) // \033[y;xH    — position cursor
void vo_color(int fg)      // \033[3Xm      — set foreground colour (0-7)
void vo_reset()            // \033[0m       — reset colours
void vo_hide_cursor()      // \033[?25l
void vo_show_cursor()      // \033[?25h
void vo_raw_mode()         // tcsetattr — non-blocking single-char input
void vo_restore_mode()     // tcsetattr — restore terminal on exit
int  vo_getch()            // read(0,&c,1) — returns char or -1 if no key
```

**Step 3 — VO descriptor (`interp/lib/term.vo`)**
- Descriptor hash + `bind_lib` call
- Exposes `term` hash with all shim functions bound

**Result — minimal terminal game loop:**
```
@ "lib/term.vo"

term.raw_mode()
term.hide_cursor()
~{
    k = term.getch()
    ? k == 119 { dy := -1 }    // w
    ? k == 115 { dy :=  1 }    // s
    ? k == 97  { dx := -1 }    // a
    ? k == 100 { dx :=  1 }    // d
    ? k == 113 { \ }           // q — quit
    // update, draw...
    term.clear()
    term.goto(x, y)
    printf_s("%s", "O")
}
term.show_cursor()
term.restore_mode()
```

### `lib/term.vo` — XY and colour interface
- DONE

VO-side API exposed by `term` hash once the shim is built:

| Call | Effect |
|------|--------|
| `term.clear()` | clear screen, cursor to 0,0 |
| `term.goto(x, y)` | move cursor to column x, row y |
| `term.color(n)` | set foreground colour 0=black 1=red 2=green 3=yellow 4=blue 5=magenta 6=cyan 7=white |
| `term.bg(n)` | set background colour (same 0-7 palette) |
| `term.reset()` | reset fg+bg to terminal default |
| `term.hide_cursor()` | hide cursor for clean animation |
| `term.show_cursor()` | restore cursor on exit |
| `term.raw_mode()` | enable non-blocking single-char input |
| `term.restore_mode()` | restore normal terminal input on exit |
| `term.getch()` | returns keycode int or -1 if no key pressed |

Colour convenience names (plain VO bindings in `lib/term.vo`):
```
term.BLACK=0  term.RED=1    term.GREEN=2  term.YELLOW=3
term.BLUE=4   term.MAGENTA=5 term.CYAN=6  term.WHITE=7
```

Example — draw a coloured character at a position:
```
@ "lib/term.vo"

term.goto(10, 5)
term.color(term.RED)
printf_s("%s", "@")
term.reset()
```

Example — fill a box:
```
fill_box = (x : int, y : int, w : int, h : int, col : int, ch : string) {
    term.color(col)
    row : int := 0
    loops.for(0, h, (row : int) {
        loops.for(0, w, (c : int) {
            term.goto(x + c, y + row)
            printf_s("%s", ch)
        })
    })
    term.reset()
}
```
