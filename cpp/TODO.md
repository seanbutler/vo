# TODO


### Interpreter / Execution 
- Shorten Overall Code Length - Seems Excessive for a Small Language


### Execution Efficiency
 - Execution Speed Benchmark Framework
 - Execuion Memory Metrics Report
 - AST Visualisation
 - Memory Visualisation
 

### Loop syntax: `~` / `loop` + `!` break (MUST SOONER) 
- Add `~` (aliased with `loop`) as a prefix loop construct: `~ cond { body }`
- Add `!` as a break statement that exits the nearest enclosing `~`
- `!` is implemented as a `BreakSignal` C++ exception, caught at the `~` boundary (mirrors `ReturnSignal`)
- Conditional break pattern: `? X { ! }` inside a loop body

```
~ true {
    code here
    ? x == 0 { ! }
}
```

### Bare block `{ }` as zero-arg callable (COULD LATER)
- A `{ body }` in expression position with no leading param list is sugar for `() { body }`
- Makes `func_name = { code }` a callable, invoked as `func_name()`
- Currently `{ }` is always parsed as a hash literal — parser needs to distinguish

### Tail-call optimisation (TCO)
- The interpreter currently uses the C++ call stack for recursion
- Deep recursion (e.g. `range(1, 10000)`) will stack overflow
- Options: trampoline in `call_callable`, or explicit continuation passing
- Intentionally deferred — only needed if large recursion depths become a use case
- The Importance of this is closely related to Loops  

### Enforce immutability at runtime (MUST LATER)
- `DeclStmt` already carries `is_mutable` flag but the interpreter does not enforce it
- `set()` on an immutable binding should throw `RuntimeError`
- Requires tracking mutability in `Environment` alongside the value

### Output / stdio design (decide and implement)

Current `printf_s`/`printf_i` are problematic. Type already encoded in the name, format string adds no value. Three options to choose from:

  1. **Simple:** bind `puts(str)` for strings + a thin C wrapper `print_int(int)` — callers never see a format string
  2. **Better:** full varargs `printf(fmt, ...)` FFI support — useful when padding/alignment/precision matter
  3. **Current:** `printf_i("%d\n", n)` style — neither simple nor powerful. Fix this.

### Configurable symbol table (tokenizer externalisation)
- Move operator/keyword symbol mappings out of the hardcoded lexer into an external table (JSON or similar)
- Allows users to remap or alias symbols to their own preferred glyphs (e.g. replace `?` with `если`, or `<-` with `≔`)
- Loaded at interpreter startup; falls back to built-in defaults if no config present
- Requires the lexer to drive tokenisation from the table rather than a fixed switch/map
- Consider: which symbols are fixed structural delimiters (braces, parens, comma) vs remappable operators

### UTF-8 support
- The lexer currently treats source as raw bytes; multi-byte UTF-8 sequences in identifiers or string literals may be mishandled
- Goals:
  - Identifiers may contain UTF-8 characters (enables non-ASCII variable/function names)
  - String literals preserve UTF-8 content correctly
  - Line/column tracking counts Unicode codepoints, not bytes
  - Operator symbols from the configurable symbol table may themselves be multi-byte glyphs
- Approach: decode source to codepoints before lexing, or use a UTF-8-aware character classification step

### Visitor-style dispatch (refactor)
- Replace `dynamic_cast` chains in `Interpreter::eval` / `Interpreter::exec` with a proper visitor pattern
- Introduce AST visitor interfaces for expressions and statements
- Add `accept(...)` methods to all AST node types
- Migrate incrementally, keep behaviour parity with existing tests
- Remove dynamic-cast chains once all nodes dispatch through visitors
- Goal: clearer extension path and stronger compile-time coverage when adding AST nodes
- Intentionally deferred until feature set is more stable

