---
status: PENDING
priority: (MUST, LATER, HIGH, BROAD)
depends: visitor-dispatch.md (clean traversal/eval API needed), homoiconicity.md (AST hash schema)
related: homoiconicity.md, publication-roadmap.md (Paper 1)
---

# AST construction API

## What this is

`parse`/`eval` (see `homoiconicity.md`) work at the string boundary — source goes in, the lexer and parser run, an AST comes out. This goes one layer deeper: expose the AST construction functions directly so VO programs can build and evaluate AST structures without ever touching a string.

```
string  ->  Lexer  ->  Parser  ->  AST nodes  ->  Interpreter  ->  values
                                    ^
                         this API lives here
```

AST nodes are already hashes (the schema is defined in `homoiconicity.md`). The construction functions are just wrappers that guarantee valid shapes. The evaluator accepts any hash that matches the schema.

## The three-layer API

### Layer 1 — node constructors (`lib/ast.vo`)

```vo
# "lib/ast.vo"

// literals
ast.int(42)
ast.float(3.14)
ast.string("hello")
ast.ident("x")

// expressions
ast.binary("+", ast.ident("x"), ast.int(1))
ast.unary("!", ast.ident("done"))
ast.call(ast.ident("f"), { ast.int(3)  ast.int(4) })
ast.member(ast.ident("point"), "x")
ast.cond(cond_node, then_block, else_block)
ast.loop(body_block)
ast.break_node()

// declarations / statements
ast.decl("x", 0, ast.int(42))          // immutable
ast.decl("x", 1, ast.int(42))          // mutable (1 = mutable flag)
ast.assign(ast.ident("x"), ast.int(99))

// hash and callable literals
ast.hash({ x = ast.int(0)  y = ast.int(0) })
ast.callable({ name="a" type="int"  name="b" type="int" }, body_block)
```

All constructors return plain VO hashes — the same schema `parse()` produces. No magic.

### Layer 2 — environment manipulation

```vo
env   = ast.env_new()              // fresh empty scope
child = ast.env_child(env)         // child inherits parent bindings
cur   = ast.env_current()          // get the currently executing environment

ast.env_bind(env, "x", 42)         // bind name -> value
ast.env_set(env, "x", 99)          // mutate existing binding
ast.env_get(env, "x")              // -> 42
ast.env_has(env, "x")              // -> 1 / 0
```

### Layer 3 — evaluation

```vo
result = ast.eval(node, env)           // evaluate one node in an environment
result = ast.eval_block(nodes, env)    // evaluate a sequence, return last value
```

## What this enables

**`parse` and `eval` become library functions, not builtins**

```vo
eval = @(src) {
    ast.eval(ast.parse_string(src), ast.env_current())
}
```

**Macros without string manipulation**

Build AST nodes directly — no string templating, no escaping, invalid structure caught at construction time not eval time:

```vo
// generate a counted loop AST programmatically
make_for = @(lo  hi  body) {
    i   = ast.decl("_i", 1, lo)
    inc = ast.assign(ast.ident("_i"), ast.binary("+", ast.ident("_i"), ast.int(1)))
    ast.eval(
        ast.loop(ast.cond(ast.binary(">=", ast.ident("_i"), hi), ast.break_node(),
                 ast.eval_block({ body  inc }, ast.env_current()))),
        ast.env_child(ast.env_current())
    )
}
```

**Sandboxed evaluation**

```vo
sandbox = ast.env_new()                  // empty — no stdlib, no builtins
ast.env_bind(sandbox, "print", print_fn) // allow only what you explicitly grant
ast.eval(ast.parse_string(user_code), sandbox)
```

**Scope injection — evaluate a callable's body with injected bindings**

```vo
ctx = ast.env_child(ast.env_current())
data >> @(k v) { ast.env_bind(ctx, k, v) }  // inject hash slots as local names
ast.eval(template_body, ctx)
```

**AST transformation pipelines**

```vo
tree = ast.parse_string(src)

// rewrite binary nodes: flatten left-to-right under explicit precedence
rewrite_prec = @(node  prec) { ... }  // walk + restructure

ast.eval(rewrite_prec(tree, prec_table), ast.env_current())
```

## Why this is better than string `eval`

| | String `eval` | AST construction API |
|---|---|---|
| Escaping / injection risk | Yes | None — values are nodes, not strings |
| Know structure at build time | No | Yes — constructors validate shape |
| Splice computed values | Via string concat | Direct node insertion |
| Control scope precisely | Limited | Exact — pass any `env` |
| Build optimized structures | No | Yes — skip unnecessary nodes |
| Readable generated code | Hard | The hash *is* the structure |

## Implementation

**Step 1 — AST node constructors as VO callables**

The schema is already in `homoiconicity.md`. The constructors are plain VO callables that return correctly-shaped hashes. No C++ changes required for this step — implement entirely in `lib/ast.vo` once `homoiconicity.md` Step 1 (AST serialiser) is done.

**Step 2 — environment API**

Requires exposing `Environment` through either:
- Native builtins (`ast_env_new`, `ast_env_child`, `ast_env_bind`, ...) in `register_builtins()`
- Or `libvo.so` C API with `"pointer"` type via FFI (depends on `sdl3-binding.md` Step 1)

The native builtin path is simpler and the right first step.

**Step 3 — `ast.eval` builtin**

`ast.eval(node, env)` deserialises the hash node (inverse of the serialiser in `homoiconicity.md` Step 3) and calls `Interpreter::eval()` with the supplied environment. One native builtin.

**Step 4 — `lib/ast.vo`**

Wire the constructors, env API, and eval together into a single importable library. `parse`/`eval` at the string level are now thin wrappers in this file rather than special builtins.

## Relation to `homoiconicity.md`

`homoiconicity.md` covers the string-level surface (`parse`/`eval`). This file covers the layer below — the construction and evaluation API those builtins are built on top of. Implement this first; `parse`/`eval` follow for free as `lib/ast.vo` functions.

## Relation to Paper 1

The emergence chain from `publication-roadmap.md`:

> one primitive (hash) -> AST nodes -> construction API -> macros -> full metaprogramming

The AST construction API is the mechanism that makes Paper 1's "emergence" argument concrete and demonstrable.
