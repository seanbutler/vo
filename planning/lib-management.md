---
status: PENDING
priority: (MUST, SOONER, MEDIUM, BROAD)
---

# Library management

## Problem 1: Paths are relative to CWD, not the importing file

`stdio.vo` contains `@ "lib/ffi.vo"` - that path only works if you run `./vo` from the `interp/` directory. From anywhere else, every import in every lib file breaks. 

**Suggestion:** Resolve import paths relative to the importing file's location, not the process CWD?


## Problem 2: No import guards

`stdio.vo` imports `ffi.vo`. If the program also imports `ffi.vo`, it runs twice. So currently every binding gets redefined, every C library gets reloaded. 

**Solition?:** The interpreter needs a canonical-path cache. If a file has already been evaluated, skip it.


### Syntax: `@` as an expression

The most VO-idiomatic fix is to make `@` (or `#` after the sigil swap) an expression that returns the module's last value, with automatic idempotency via the path cache:

```vo
loops = @ "lib/loops.vo"    // execute, cache, return the hash,  bind to loops

loops = @ "lib/loops.vo"    // return cached hash - file not re-executed
```

The interpreter caches `{ canonical_path → returned_value }`. The import guard is automatic
and invisible. Using `@` as a statement still works (return value discarded), so existing
code is unaffected:

```vo
@ "lib/loops.vo"    // still valid — idempotent, return value discarded
```

This also kills scope flooding (Problem 5) — the importer controls the name, the lib's
bindings don't leak.

### Force-reload: `@@`

For the rare case where re-execution is genuinely wanted (hot-reload during development,
generative libs), a double sigil bypasses the cache:

```vo
@@  "lib/mylib.vo"   // always re-executes — ignores cache
```

Consistent with the `>>` / `:=` doubling pattern already in the language.

### Other options considered

**Option: guard via `_imports` hash** — the interpreter exposes a `_imports` hash keyed by
canonical path; a guard is plain VO code:

```vo
? _imports.("lib/ffi.vo") { } { @ "lib/ffi.vo" }
```

Wrappable as a stdlib callable. Logical but verbose, and requires `@` to accept an
expression (wanted anyway — see Problem 3).

**Option: guard slot inside the lib file** — lib declares its own guard. Requires a new
"skip rest of file" mechanism; `\` is a loop escape, not a file escape. Discarded.

## Problem 3: Hardcoded `"lib/"` prefix everywhere

Every file in your project has to know where `lib/` lives relative to itself. Fix: a search
path — an ordered list of directories the interpreter checks before treating the path as
literal. Standard convention:

```
1. project ./lib/
2. project ./vendor/
3. VO installation stdlib
```

Set via `VO_PATH` env var or an interpreter flag. Short names then work:

```vo
@ "stdio"       // found in stdlib — no path prefix needed
@ "mymodule"    // found in ./lib/
```

## Problem 4: No separation of stdlib, project, and external libs

Everything lives in `interp/lib/` — the interpreter's own stdlib is mixed with the idea of
user project libs. Cleaner three-tier layout:

| Tier | Location | Purpose |
|---|---|---|
| Stdlib | interpreter installation | ships with `vo`, always on path |
| Project | `./lib/` | your program's own libraries |
| Vendor | `./vendor/` | pinned copies of external libs |

## Problem 5: Inconsistent export style

Currently mixed — some libs namespace cleanly, some flood:

```vo
@ "lib/loops.vo"    // adds `loops` hash — namespaced ✓
@ "lib/stdlib.vo"   // adds clone, merge, subtype, ... directly — floods scope ✗
@ "lib/stdio.vo"    // adds printf_s, printf_i directly — floods scope ✗
```

The convention `loops` already uses is the right one. Every lib file's last expression should
be the public hash. The importer names it:

```vo
stdio  = @ "stdio"     // printf lives at stdio.printf_s
stdlib = @ "stdlib"    // merge lives at stdlib.merge
```

This also means the importer controls the name — no collisions, no surprises.

## Problem 6: No directory modules

Once libs grow, a single flat file isn't enough. Convention: `@ "mylib"` loads
`mylib/index.vo` if it exists, `mylib.vo` otherwise. Index file assembles and re-exports
the parts:

```
lib/
  mylib/
    index.vo      ← assembles and exports the public hash
    internals.vo
    helpers.vo
```

## Implementation order

| Change | Effort | Impact |
|---|---|---|
| Import path relative to importing file | Small — interpreter only | Fixes silent bugs today |
| `@` as expression returning module hash + path cache | Small — interpreter only | Import guards + kills scope flooding |
| `@@` force-reload sigil | Trivial — one extra token | Hot-reload during development |
| Search path (`VO_PATH`) | Small — interpreter + convention | Removes hardcoded prefixes |
| Three-tier layout (stdlib / lib / vendor) | Zero code — pure convention | Immediate clarity |
| Consistent export style (every lib returns a hash) | Medium — refactor existing libs | Clean namespacing |
| Directory modules (`index.vo`) | Small — import resolver change | Scales to larger libs |

The first three are interpreter changes that unblock everything else.
