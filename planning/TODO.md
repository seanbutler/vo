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

### Homoiconicity — `parse` and `eval` builtins
- (COULD, LATER, HIGH, BROAD)
- PENDING

**Vision**

VO's AST is exposed as VO hashes. Code becomes data; data becomes executable. Two builtins enable this:

- **`parse(s : string)`** — lexes and parses a VO source string, returns the AST as a VO hash
- **`eval(node)`** — takes a VO hash AST and executes it in the current environment

```vo
tree = parse("1 + 2")
// tree = { type = "binary"  op = "+"  left = { type = "int"  value = 1 }  right = { type = "int"  value = 2 } }

eval(tree)   // → 3
```

Once these exist, operator precedence reordering is expressible entirely in VO:

```vo
prec = { + = 6  - = 6  * = 7  / = 7  % = 7 }

reorder = @(node) {
    // walk node, restructure binary subtrees according to prec hash
    ...
}

eval(reorder(parse("1 + 2 * 3")))   // → 7, not 9
```

Any syntax transform becomes a VO library — macros, precedence, DSLs, code generation — without touching the interpreter.

**AST hash schema**

| Node type | Hash shape |
|-----------|-----------|
| Integer   | `{ type = "int"  value = n }` |
| Float     | `{ type = "float"  value = f }` |
| String    | `{ type = "string"  value = s }` |
| Identifier | `{ type = "ident"  name = s }` |
| Binary    | `{ type = "binary"  op = s  left = node  right = node }` |
| Unary     | `{ type = "unary"  op = s  operand = node }` |
| Call      | `{ type = "call"  callee = node  args = { ... } }` |
| Hash literal | `{ type = "hash"  members = { ... } }` |
| Callable  | `{ type = "callable"  params = { ... }  body = { ... } }` |
| Cond      | `{ type = "cond"  cond = node  then = { ... }  else = { ... } }` |
| Decl      | `{ type = "decl"  name = s  mutable = int  value = node }` |
| Assign    | `{ type = "assign"  target = node  value = node }` |

**Implementation**

*Step 1 — AST serialiser*: walk the existing C++ AST nodes and emit VO hash values. One visitor method per node type. No new AST nodes required.

*Step 2 — `parse` builtin*: calls the existing `Lexer` + `Parser`, then the serialiser. Returns a `ValuePtr` hash.

*Step 3 — AST deserialiser*: walk a VO hash and reconstruct C++ AST nodes. Inverse of the serialiser.

*Step 4 — `eval` builtin*: calls the deserialiser then `Interpreter::eval()` on the result.

**Breaking change**: none — additive only.


### Unified dot / infix / prefix — one dispatch rule
- (MUST, LATER, HIGH, BROAD)
- PENDING

**Insight**

Dot notation and infix operators are the same transformation — first argument moves left of the function name:

```
f(a, b)   →   a.f(b)    // dot notation
f(a, b)   →   a + b     // infix operator
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

Operator overloading falls out for free — add a `+` slot to any hash and it intercepts `a + b` for that type:

```vo
Vec = {
    x : int = 0
    y : int = 0
    + = @(other) { Vec(self.x + other.x  self.y + other.y) }
}
v1 = Vec(1  2)
v2 = Vec(3  4)
v3 = v1 + v2    // a.+(b) → looks up + on v1 → Vec.+
```

UFCS means dot and prefix are interchangeable when the name is not a member:

```vo
double = @(x : int) { x * 2 }
5.double()     // looks up double on 5 (int), not found, falls through to global → double(5)
```

**The single dispatch rule**

> `.` means: look up the name starting from the left operand's prototype chain, then global scope; pass the left operand as the first argument.

Infix operator syntax is `.` with the function name written between operands instead of after the dot. They are one rule, not two.

**Consequences**

- Dot notation, infix operators, and UFCS unify into one mechanism
- Operator overloading requires no new language feature — just a `+` slot on a hash
- The symbol table shrinks — `.` and infix dispatch are the same rule
- Prefix `f(a, b)` bypasses receiver lookup entirely — explicit global call
- Complements operators-as-callables (see below): once `+` is a callable identifier, `a.+(b)` is already valid syntax

**Implementation**

Depends on:
- Operators as first-class callables (operator glyphs as identifiers)
- Flat binary expression parser (no hardcoded precedence ladder)

*Step 1*: extend member lookup in `parse_postfix()` — after `.name`, if followed by `(`, pass left operand as implicit first argument alongside explicit args.

*Step 2*: extend infix operator rewrite — `a op b` rewrites to `a.op(b)` rather than `op(a, b)`; receiver lookup attempted first, global fallback second.

*Step 3*: update `HashInstance::get` / interpreter call path to support the fallthrough from receiver to global scope.

*Step 4*: update `self` binding — `self` is bound to the receiver (left operand) when lookup succeeds on the prototype chain; not bound on global fallthrough.


### Operators as first-class callables
- (MUST, LATER, HIGH, BROAD)
- PENDING

**Vision**

Arithmetic operators `+` `-` `*` `/` `%` become pre-bound callable identifiers. Infix syntax is purely a parser rewrite — `a + b` desugars to `+(a, b)` at parse time. Users can shadow, redefine, or delegate operators like any other callable. Combined with the localisation symbol table, any operator glyph becomes remappable.

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

**Precedence decision**

The current parser encodes precedence structurally (multiplicative ladder sits above additive). If operators become plain callables this structure must change. Two options:

1. **No precedence — explicit parentheses required**
   - `3 + 4 * 2` becomes a parse error or left-to-right: must write `3 + (4 * 2)`
   - Consistent with VO's philosophy — no hidden rules, programmer states intent explicitly
   - Simplifies the parser significantly (flat binary expression level)
   - **Preferred option**

2. **Precedence metadata on callables**
   - Each operator callable carries a `_prec` slot (e.g. `*.prec = 7  +._prec = 5`)
   - Parser reads this at parse time to order rewrites
   - Complex to implement; mixes runtime values into parse-time decisions

**Preferred: adopt no precedence, require explicit parentheses.** This is consistent with VO's principle that the programmer states intent explicitly — the same reason there is no implicit return ambiguity, no hidden coercion, and no reserved words.

---

**Step 1 — infix rewrite for named functions (prerequisite)**

Already in TODO as "User-defined infix call syntax". Implement `` a `f` b `` → `f(a, b)` first. This establishes the parse-time rewrite infrastructure that operator desugaring will reuse.

**Step 2 — make operator glyphs valid identifier starts in the lexer**

Currently `+` `-` `*` `/` `%` are single-char tokens consumed before the identifier path. Extend the lexer so that a bare `+` (not followed by a digit or another operator) is tokenised as an `Identifier` with lexeme `"+"`. Requires careful ordering in `tokenize()` to avoid breaking numeric literals and existing operator tokens.

**Step 3 — flatten the parser precedence ladder**

Replace the `parse_additive` / `parse_multiplicative` / `parse_comparison` chain with a single `parse_binary()` level. All infix operator tokens (`+` `-` `*` `/` `%` `==` `!=` `<` `<=` `>` `>=`) rewrite to calls at the same precedence — left to right. Mixed expressions without parentheses either parse left-to-right or are a parse error (decision: parse error preferred, forces explicitness).

**Step 4 — bootstrap operator bindings**

Pre-bind `+` `-` `*` `/` `%` in the interpreter's `register_builtins()` (or a `lib/operators.vo`) as native callables wrapping the existing C++ arithmetic. These are ordinary environment bindings — shadowing them replaces the operator for that scope.

**Step 5 — comparison operators**

Same treatment for `==` `!=` `<` `<=` `>` `>=` — each becomes a pre-bound callable. This enables user-defined equality and ordering for custom hash types.

**Step 6 — remove operator tokens from the token set**

Once all operator glyphs are identifiers, `TT::Plus`, `TT::Minus`, `TT::Star`, `TT::Slash`, `TT::Percent` are removed from the token type enum. The lexer symbol table shrinks by 5+ entries. The comparison token types follow once Step 5 is complete.

**Result**

```vo
# "lib/operators.vo"    // provides + - * / % == != < <= > >=

x = (3 + (4 * 2))      // explicit parentheses required — no precedence
y = 3 + 4 * 2          // parse error: ambiguous without parentheses

// operator overloading via hash slot
Vec = {
    x : int = 0
    y : int = 0
    + = @(other) { Vec(self.x + other.x  self.y + other.y) }
}
v1 = Vec(1  2)
v2 = Vec(3  4)
v3 = v1 + v2            // desugars to +(v1, v2) → Vec.+ lookup via self
```

**Breaking change**: all existing expressions using `+` `-` `*` `/` `%` without explicit parentheses around mixed operators will need parentheses added.


### Drop comma from parameter lists
- (MUST, SOONER, LOW, BROAD)
- PENDING

Parameter lists currently require commas between parameters:

```vo
add = @(a : int, b : int) { a + b }
```

Commas are not needed — parameters are always `name` or `name : type`, both starting with an identifier, so whitespace separation is unambiguous. Hash members already use whitespace separation with no comma; dropping commas from parameter lists makes the language fully consistent:

```vo
add = @(a : int  b : int) { a + b }
```

Call arguments retain commas for now — `f(a -b)` is ambiguous without them (unary minus vs binary minus).

**Implementation — parser only**

- In `parse_param_list()`, replace `while (match(TT::Comma))` with a loop that continues as long as the next token is an identifier (i.e. the start of a new parameter) and not `TT::RParen`
- No lexer changes; no AST changes; no runtime changes

**Migration**

- Remove all commas from parameter lists in `interp/lib/`, `interp/tests/`, `interp/examples/`, `interp/game/`
- Mechanical find-and-replace; existing tests confirm correctness after

**Breaking change**: all callables with multiple parameters need commas removed from their parameter lists.


### Swap `@` / `#` — callable sigil and import sigil
- (MUST, SOONER, LOW, BROAD)
- PENDING

**Motivation**

Callables and grouped expressions are currently ambiguous — both start with `(`.
The parser resolves this with a lookahead heuristic (`looks_like_callable()`: scan to matching `)`, check if `{` follows). This is fragile and prevents `(expr) { hash }` from ever being written as two adjacent statements.

A callable sigil eliminates the ambiguity with zero lookahead. `@` is the natural choice; `#` is the natural choice for import.

**The swap**

| Today | After |
|-------|-------|
| `@ "lib/stdio.vo"` | `# "lib/stdio.vo"` |
| `(x, y) { x + y }` | `@(x, y) { x + y }` |

- **`#`** — universally read as preprocessor / include / meta (`#include`, `#import`). Zero ambiguity with any future operator.
- **`@`** — already carries "at / address / callable" semantics (Python decorator + matrix multiply, Ruby instance vars). `@(x) { body }` reads naturally as a callable literal.

**Implementation**

*Step 1 — lexer*: `@` already emits `TokenType::At`. No token type change needed — the parser just gains a new interpretation for `At`.

*Step 2 — parser `parse_primary()`*:
- Replace the `check(TT::At)` import branch: `#` (`TT::Hash`, new token) triggers `parse_import()`
- Add `check(TT::At)` branch *before* the `LParen` check: consume `@`, then call `parse_callable()` directly — no lookahead needed
- Remove `looks_like_callable()` and its call site entirely

*Step 3 — parser `parse_stmt()`*:
- Import now triggered by `TT::Hash` instead of `TT::At`

*Step 4 — lexer*:
- Add `#` → `TT::Hash` token (currently `#` hits `default: error(...)`)

*Step 5 — migrate all `.vo` files*:
- Global find-and-replace `@ "` → `# "` across `interp/lib/`, `interp/tests/`, `interp/examples/`, `interp/game/`
- Mechanical; no logic changes

**Result**

```vo
# "lib/stdio.vo"         // import — reads like #include
# "lib/loops.vo"

add = @(a : int, b : int) { a + b }   // callable — unambiguous
f = @(x) { x * 2 }
result = f(21)
```

Grammar becomes unambiguous with one token of lookahead. `looks_like_callable()` deleted. `#` is safe from future operator conflicts; `@` has no plausible future use as an arithmetic operator.

*Step 6 — extend test coverage*:
- Existing tests cover callable *behaviour* but not callable *syntax* as a distinct form
- Add cases to `tests/test_unicode.vo` or a new `tests/test_callable_syntax.vo` that explicitly exercise `@(x) { }` in positions that previously relied on `looks_like_callable()`:
  - Zero-param callable: `@() { 42 }`
  - Callable immediately followed by a hash literal (the previously ambiguous case): `f = @(x) { x }  data = { key = 1 }`
  - Callable as a hash member value
  - Callable passed as an argument

**Breaking change**: all existing `.vo` source files need the mechanical `@ "` → `# "` substitution.


### Interpreter Runtime Controls

We need to set controls (flags?) in the interpreter

**Choices:**
- Confguration File for Defaults
- Override Via Flags For Specific Calls of Interpreter

**Options:**
- Vo Lib @ include Folder
- config for c linking details
- Visualisations
- Optimisations





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


### Replace hardcoded NativeCallable builtins with FFI + external libs
- (COULD, SOONER, MEDIUM, BROAD)
- PENDING

`ifloor` and `char_at` are currently special-cased as `NativeCallable` objects wired directly into the interpreter in `register_builtins()`. They exist because the FFI dispatcher cant express their signatures. 

The goal is to remove them and use ordinary `bind_lib`-bound C functions.

**What is blocking them today**

| Builtin | Blocker |
|---|---|
| `ifloor(double) → int` | No `double` param type in the integer-register-class dispatcher |
| `char_at(string, int) → string` | No `cstring` return type — dispatcher can only return `int64_t` |

**Step 1 — extend the FFI dispatcher (`interpreter.cpp`)**
- Add `"double"` as a recognised param type in the integer-register-class path (reinterpret `double` bits to `int64_t` for passing, interpret result accordingly)
- Add `"cstring"` as a return type: cast the `int64_t` result to `const char*` and wrap in `Value::from(std::string(...))`

**Step 2 — C shim library (`interp/vendor/vostr/vostr.c`, compiled to `libvostr.so`)**
```c
#include <math.h>
#include <string.h>

// char_at: returns a pointer to a 1-char null-terminated static buffer.
// Safe for immediate use; caller must not store the pointer across calls.
static char vo_char_at_buf[2];
const char* vo_char_at(const char* s, int index) {
    vo_char_at_buf[0] = s[index];
    vo_char_at_buf[1] = '\0';
    return vo_char_at_buf;
}

int vo_ifloor(double x) { return (int)floor(x); }
```

**Step 3 — VO descriptor (`interp/lib/cstring.vo` and `interp/lib/cmath.vo`)**
```
// cstring.vo  (char_at is a language builtin — replaced by:)
vostr_lib = { lib : string = "libvostr.so"  abi : string = "c"
    functions = {
        char_at = { symbol : string = "vo_char_at"
                    params  = { p1 : string = "cstring"  p2 : string = "int" }
                    returns : string = "cstring" }
    }
}
vostr = bind_lib(vostr_lib)
char_at = (s : string, i : int) { vostr.char_at(s, i) }
```

```
// in cmath.vo — add alongside sin/cos/etc:
        ifloor = { symbol : string = "vo_ifloor"
                   params  = { p1 : string = "double" }
                   returns : string = "int" }
```

**Step 4 — remove from interpreter**
- Delete the `ifloor` and `char_at` blocks from `Interpreter::register_builtins()`
- Update `lib/cstring.vo` comment (currently reads "char_at is a language builtin, not listed here")

**Result — usage unchanged from caller perspective:**
```
@ "lib/cmath.vo"
@ "lib/cstring.vo"

row : string = "1011100001111001"
cell : string = char_at(row, 3)        // "1"
y : int = ifloor(3.9)                  // 3
```


### Lazy boolean operators `&` and `|`
- (MUST, SOONER, SMALL, BROAD)
- PENDING

- Add `&` (logical AND) and `|` (logical OR) as proper infix operators
- `a & b` desugars to `? a { b } { 0 }` — short-circuits, no call-frame overhead
- `a | b` desugars to `? a { 1 } { b }` — short-circuits
- Precedence: `|` below `&`, both below `!`, above comparison


### User-defined infix call syntax 
- (COULD, SOONER, SMALL, BROAD)
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


### Localisation — three-layer architecture 
- (MUST, SOONER, HIGH, BROAD)
- PENDING

VO has no reserved words and symbol-only syntax, making it uniquely suited to full natural-language localisation. Though its not just localisation its also a general customisation tech and perhaps also a way to program and build abstractions.

The goal is to separate three concerns cleanly:

```
1. Source (community symbols) → symbol table → canonical AST
2. Canonical AST → interpreter
3. Interpreter errors → error template table → community-language diagnostics
```

**Layer 1 — UTF-8 support (lexer + runtime)**
- Lexer currently treats source as raw bytes; multi-byte sequences may be mishandled
- Identifiers may contain UTF-8 characters (non-ASCII variable/function names)
- String literals preserve UTF-8 content correctly
- Line/column tracking counts Unicode codepoints, not bytes
- Operator glyphs in the symbol table may themselves be multi-byte
- Approach: decode source to codepoints before lexing, or UTF-8-aware character classification

**Layer 2 — Error message templates (runtime)**
- Externalise all `RuntimeError` and `ParseError` strings into a template table
- Templates must support:
  - Argument reordering (Arabic, Japanese, Turkish word order)
  - Gender agreement metadata
  - Plural forms (Russian 4 forms, Arabic 6 forms)
  - RTL/LTR direction hints
- Consider Mustache-style templates for community accessibility
- Library options: mstch, kainjow/mustache, or std::format (C++20)


### Enforce immutability at runtime 
- (MUST, LATER, LOW, BROAD)
- PENDING

- `DeclStmt` already carries `is_mutable` flag but the interpreter does not enforce it
- `set()` on an immutable binding should throw `RuntimeError`
- Requires tracking mutability in `Environment` alongside the value


### Output / stdio design 
- (MUST, LATER)
- PENDING

Current `printf_s`/`printf_i` are problematic. Type already encoded in the name, format string adds no value. Three options to choose from:

  1. **Simple:** bind `puts(str)` for strings + a thin C wrapper `print_int(int)` — callers never see a format string
  2. **Better:** full varargs `printf(fmt, ...)` FFI support — useful when padding/alignment/precision matter
  3. **Current:** `printf_i("%d\n", n)` style — neither simple nor powerful. Fix this.


### Publication roadmap
- (MUST, SOONER, LOW, BROAD)
- PENDING

Four papers have been identified from VO's design. They have a dependency order — earlier papers provide engineering foundations for later ones.

```
Paper 3  (cache-aware AST layout)          ← implement and publish first
    ↓  provides: arena allocator, visit counter, benchmark suite
Paper 4  (shared profiling architecture)   ← needs bytecode VM added
    ↓  provides: profile export format, end-to-end pipeline
Paper 1  (language design / emergence)     ← needs homoiconicity, unified dispatch
Paper 2  (theory / hash rewriting)         ← speculative; pursue in parallel with 1
```

**Paper 3** — Cache-aware AST layout *(VEE / ISMM / DLS)*
- Arena allocator replacing `std::shared_ptr` heap allocation
- Visit counter per AST node
- Runtime cache size query (L1/L2/L3)
- Hot subgraph compaction into cache-tier regions
- Benchmark: scattered vs static flat vs runtime-guided
- Most immediately actionable; confirmed gap in literature

**Paper 4** — Shared profiling architecture across staged runtimes *(OOPSLA / MoreVMs / ManLang)*
- Depends on Paper 3 + bytecode VM
- Profile export format (visit counts, type observations, branch frequencies)
- Bytecode compiler consuming profile: inlining, reordering, branch layout
- JIT consuming profile: inline caches, type specialisation, register hints
- Key claim: self-improving profiling — the profiler bootstraps its own performance

**Paper 1** — Language design: emergence from one primitive *(Onward! Essays / DLS)*
- Depends on homoiconicity (`parse`/`eval`) being implemented
- Depends on unified dispatch (dot/infix/UFCS) being implemented
- The emergence chain: one primitive → many mechanisms at no additional conceptual cost

**Paper 2** — Theory: hash/feature-structure rewriting *(LICS / FSCD / workshop)*
- Speculative; pursue in parallel with Paper 1
- Requires: confluence proofs, termination conditions, expressiveness results
- May produce a formal foundation beneath the practical contributions of Papers 1 and 3/4

See `planning/PAPER.md` for full details on each paper.


### Cache-aware AST layout
- (COULD, LATER, MEDIUM, LARGE)
- PENDING

**Motivation**

A tree-walking interpreter's main bottleneck is pointer chasing — each node visit follows pointers to children scattered randomly in heap memory. An L1 cache hit costs ~4 cycles; a main memory access costs ~200 cycles. If the hot subgraph of the AST fits in L1/L2 cache, the interpreter runs significantly faster with no change to the execution model.

**The mechanism**

*Step 1 — query cache sizes at runtime*

On Linux: `/sys/devices/system/cpu/cpu0/cache/index{0,1,2}/size`
On macOS: `sysctl hw.l1icachesize hw.l2cachesize`

Store L1 and L2 sizes in the interpreter at startup. No user configuration required.

*Step 2 — switch to arena allocation at parse time*

Currently AST nodes are individually heap-allocated `std::shared_ptr` objects — scattered in memory. Replace with a pool/arena allocator: all nodes packed into a contiguous slab. Node references become integer indices into the arena rather than pointers. This is the enabling change — reorganisation becomes array permutation rather than pointer surgery.

*Step 3 — instrument the interpreter (trace phase)*

Add a visit counter to each AST node (a single `uint32_t` in the node struct). Increment on every eval/exec call. Adds one integer increment per node visit — negligible overhead.

*Step 4 — identify the hot subgraph*

After a warmup period (configurable — e.g. first 1000 iterations of the main loop), sort nodes by visit count descending. The hot set is the top-N nodes whose total size fits within L2 (or L1 for the very hottest).

*Step 5 — compact hot nodes*

Copy hot nodes to the front of a new arena, preserving child index relationships. Update the interpreter's root pointer. Cold nodes remain in place at the back. Future allocations go into the cold region.

*Step 6 — re-run*

The interpreter continues unchanged. Hot path node visits now hit cache; cold path visits still miss but are rare by definition.

**Why not just use a bytecode VM?**

A bytecode VM gets cache locality for free — instructions are a flat array. But compiling to bytecode discards the AST, losing homoiconicity, live AST transformation, and debuggability. Cache-aware layout gets similar locality improvements while the AST remains an AST — still inspectable, transformable, and self-describing.

**Expected gains**

For tight loops over a small AST subgraph (the common case in games, simulations, inner loops): 2–5× speedup over scattered allocation. Smaller gains for programs that touch large portions of the AST uniformly.

**Relation to other work**

| Technique | Relationship |
|-----------|-------------|
| AST flattening (Sampson) | Packs pointer-based AST nodes into contiguous arrays for ~1.5× speedup — static, not runtime-driven |
| Self-optimising AST interpreters (Würthinger / Truffle) | Tree rewriting with type feedback to reduce dispatch overhead — structural, not layout |
| Hot path B+-tree layout (VLDB 2023) | Runtime identification of hot access paths + contiguous layout — same concept, applied to database indexes not interpreters |
| Profile-guided optimisation (PGO) | Same principle applied to machine code layout by compilers; this applies it to AST node layout |
| Copying GC | Same compaction technique; different motivation (reclaim memory vs improve locality) |
| JIT code layout (V8, LuaJIT) | Hot machine code placed contiguously; this does the same for AST nodes |

Research confirms no existing tree-walking interpreter applies runtime cache-size-aware hot subgraph compaction. The idea sits in a gap between three separate bodies of work that have not been combined. Potentially publishable as a standalone systems contribution.

**Performance ladder**

```
Tree-walking, scattered AST (current)
    ↓
Tree-walking, cache-aware AST layout      ← this item
    ↓
Bytecode VM
    ↓
JIT
```


### Tail-call optimisation 
- (COULD, LATER, HIGH, BROAD)
- PENDING

- The interpreter currently uses the C++ call stack for recursion
- Deep recursion (e.g. `range(1, 10000)`) will stack overflow
- Options: trampoline in `call_callable`, or explicit continuation passing
- Intentionally deferred — only needed if large recursion depths become a use case


### Profiling report 
- (COULD, SOONER, MEDIUM, LARGE)
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

### AST Visualisation via Graphviz 
- (COULD, SOONER, MEDIUM, LARGE)
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


### Visitor-style dispatch refactoring 
- (SHOULD, LATER, HIGH, LIMITED)
- PENDING

- Replace `dynamic_cast` chains in `Interpreter::eval` / `Interpreter::exec` with a proper visitor pattern
- Introduce AST visitor interfaces for expressions and statements
- Add `accept(...)` methods to all AST node types
- Migrate incrementally, keep behaviour parity with existing tests
- Remove dynamic-cast chains once all nodes dispatch through visitors
- Goal: clearer extension path and stronger compile-time coverage when adding AST nodes
- Intentionally deferred until feature set is more stable


### Code Size & Complexity 
- (COULD, LATER, MEDIUM, SMALL)
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


### null alias for `{}`  
- (COULD, SOONER, SMALL, SMALL)
- DONE

- `{}` is already the language's null/empty sentinel — used wherever "nothing" is needed
- Add a stdlib binding so users can write `null` instead of `{}`
- Simplest implementation: one line in `lib/stdlib.vo` — `null = {}`
- No language changes required; `null` is just an identifier bound to the empty hash
- FFI pointer dispatcher should treat empty hash as `NULL` (i.e. `nullptr`) — relevant for SDL3 and any C library that takes optional pointer arguments


### Bare block `{ }` as zero-arg callable 
- (COULD, LATER, SMALL, SMALL)
- DONE

A syntax change that potentially breaks backward compatability. A `{ body }` in expression position with no leading param list is sugar for `() { body }`. Makes `func_name = { code }` a callable, invoked as `func_name()`. Currently `{ }` is always parsed as a hash literal — parser needs to distinguish.


### Terminal graphics via ANSI escape codes 
- (COULD, SOONER, SMALL, LARGE)
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
