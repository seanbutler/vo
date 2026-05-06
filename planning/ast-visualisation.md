---
status: PENDING
priority: (COULD, SOONER, MEDIUM, LARGE)
depends: visitor-dispatch.md (soft — emitter is cleaner as a visitor; not strictly required)
---

# AST Visualisation via Graphviz

Render the parsed AST as a Graphviz `.dot` graph, one per source file and one combined graph for the whole program including imports.

## Trigger

`--ast` flag passed to the `vo` binary — dumps AST after parse, before execution. Automatic on each `#` import when `--ast` is active.

## Output

- Per-file: `<source_stem>.dot` (e.g. `main.vo` → `main.dot`)
- Combined: `program.dot` — all files merged into one digraph, subgraphs per file

## Step 1 — `--ast` flag (`main.cpp`)

- Parse argv for `--ast` before running; pass a bool into `Interpreter` or a new `AstPrinter`
- Strip the flag from the file argument list so the interpreter still runs normally

## Step 2 — Graphviz emitter (`src/ast/dot_emitter.hpp/.cpp`)

- Visitor over the AST (expressions + statements)
- Each node becomes a Graphviz node with a unique ID (e.g. pointer address or counter)
- Label: node type + key field (literal value, operator, identifier name)
- Edges: parent → child for each child slot
- Output: valid `.dot` file, one `digraph` per file, `subgraph cluster_<file>` in combined output

## Step 3 — Per-file emit on `#` import (`interpreter.cpp`)

- After parsing an imported file, if `--ast` active, call the emitter and write `<stem>.dot`
- Append the file's nodes/edges into the combined graph accumulator

## Step 4 — Combined graph flush (`main.cpp`)

After the full program has loaded (all imports resolved), write `program.dot`.

## Usage

```
./vo --ast mygame.vo
# produces: mygame.dot, stdlib.dot, vtkit.dot, ... program.dot
dot -Tpng program.dot -o program.png && xdg-open program.png
```
