---
status: PENDING
priority: (COULD, LATER, HIGH, BROAD)
depends: ast-api.md (parse/eval are thin wrappers over the construction API)
---

# Homoiconicity — `parse` and `eval` builtins

## Vision

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

## AST hash schema

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

## Implementation

*Step 1 — AST serialiser*: walk the existing C++ AST nodes and emit VO hash values. One visitor method per node type. No new AST nodes required.

*Step 2 — `parse` builtin*: calls the existing `Lexer` + `Parser`, then the serialiser. Returns a `ValuePtr` hash.

*Step 3 — AST deserialiser*: walk a VO hash and reconstruct C++ AST nodes. Inverse of the serialiser.

*Step 4 — `eval` builtin*: calls the deserialiser then `Interpreter::eval()` on the result.

**Breaking change**: none — additive only.
