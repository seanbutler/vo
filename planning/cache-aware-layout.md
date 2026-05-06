---
status: PENDING
priority: (COULD, LATER, MEDIUM, LARGE)
depends: visitor-dispatch.md (soft — arena walk is cleaner with a visitor; not strictly required)
related: publication-roadmap.md (Paper 3)
---

# Cache-aware AST layout

## Motivation

A tree-walking interpreter's main bottleneck is pointer chasing — each node visit follows pointers to children scattered randomly in heap memory. An L1 cache hit costs ~4 cycles; a main memory access costs ~200 cycles. If the hot subgraph of the AST fits in L1/L2 cache, the interpreter runs significantly faster with no change to the execution model.

## Steps

**Step 1 — query cache sizes at runtime**

On Linux: `/sys/devices/system/cpu/cpu0/cache/index{0,1,2}/size`
On macOS: `sysctl hw.l1icachesize hw.l2cachesize`

Store L1 and L2 sizes in the interpreter at startup. No user configuration required.

**Step 2 — switch to arena allocation at parse time**

Currently AST nodes are individually heap-allocated `std::shared_ptr` objects — scattered in memory. Replace with a pool/arena allocator: all nodes packed into a contiguous slab. Node references become integer indices into the arena rather than pointers. This is the enabling change — reorganisation becomes array permutation rather than pointer surgery.

**Step 3 — instrument the interpreter (trace phase)**

Add a visit counter to each AST node (a single `uint32_t` in the node struct). Increment on every eval/exec call. Adds one integer increment per node visit — negligible overhead.

**Step 4 — identify the hot subgraph**

After a warmup period (configurable — e.g. first 1000 iterations of the main loop), sort nodes by visit count descending. The hot set is the top-N nodes whose total size fits within L2 (or L1 for the very hottest).

**Step 5 — compact hot nodes**

Copy hot nodes to the front of a new arena, preserving child index relationships. Update the interpreter's root pointer. Cold nodes remain in place at the back. Future allocations go into the cold region.

**Step 6 — re-run**

The interpreter continues unchanged. Hot path node visits now hit cache; cold path visits still miss but are rare by definition.

## Why not just use a bytecode VM?

A bytecode VM gets cache locality for free — instructions are a flat array. But compiling to bytecode discards the AST, losing homoiconicity, live AST transformation, and debuggability. Cache-aware layout gets similar locality improvements while the AST remains an AST — still inspectable, transformable, and self-describing.

## Expected gains

For tight loops over a small AST subgraph (the common case in games, simulations, inner loops): 2–5× speedup over scattered allocation. Smaller gains for programs that touch large portions of the AST uniformly.

## Relation to other work

| Technique | Relationship |
|-----------|-------------|
| AST flattening (Sampson) | Packs pointer-based AST nodes into contiguous arrays for ~1.5× speedup — static, not runtime-driven |
| Self-optimising AST interpreters (Würthinger / Truffle) | Tree rewriting with type feedback to reduce dispatch overhead — structural, not layout |
| Hot path B+-tree layout (VLDB 2023) | Runtime identification of hot access paths + contiguous layout — same concept, applied to database indexes not interpreters |
| Profile-guided optimisation (PGO) | Same principle applied to machine code layout by compilers; this applies it to AST node layout |
| Copying GC | Same compaction technique; different motivation (reclaim memory vs improve locality) |
| JIT code layout (V8, LuaJIT) | Hot machine code placed contiguously; this does the same for AST nodes |

Research confirms no existing tree-walking interpreter applies runtime cache-size-aware hot subgraph compaction. The idea sits in a gap between three separate bodies of work that have not been combined. Potentially publishable as a standalone systems contribution (see `publication-roadmap.md`, Paper 3).

## Performance ladder

```
Tree-walking, scattered AST (current)
    ↓
Tree-walking, cache-aware AST layout      ← this item
    ↓
Bytecode VM
    ↓
JIT
```
