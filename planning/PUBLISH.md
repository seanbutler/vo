# Publication Potential

Three distinct papers can come from VO's design and implementation if we continue development. 


## Paper 1 - Language design: Emergence from a single primitive

**Venue**: Onward! Essays (SPLASH) or DLS — Dynamic Languages Symposium
**Readiness**: Requires homoiconicity and unified dispatch to be implemented first
**Claim type**: Language design / experience report

### The chain

**Hash is the only data structure** 
- AST is a hash 
- Homoiconicity is free
- Operator precedence is a VO library
- Operators are callables
- Infix and dot are one dispatch rule
- Operator overloading is free
- UFCS is free
- FFI specs are hashes
- FFI bindings are inspectable and composable

No other language derives this many features from a single primitive without adding additional concepts.

**The publishable claim:**

A language with one universal data structure doesn't just simplify the runtime, it collapses multiple traditionally separate language mechanisms (object system, module system, FFI, macro system, operator overloading, method dispatch, UFCS) into consequences of a single decision, with no additional conceptual machinery.

### Candidate A — accidental homoiconicity

In Lisp, S-expressions were *designed* to be both code and data. In VO, homoiconicity emerges *accidentally* — the language didn't set out to be homoiconic, it became homoiconic because it committed to one universal structure for independent reasons.

**Accidental homoiconicity: when a single universal data structure makes code-as-data a natural consequence rather than a design goal.**

- Hash-as-universal-structure motivated independently (objects, modules, FFI)
- AST-as-hash follows from the same decision
- `parse` and `eval` give a full macro system at no additional conceptual cost
- Operator precedence, DSLs, syntax extensions become VO libraries

### Candidate B — unified dispatch (dot / infix / UFCS)

Dot notation and infix operators are the same transformation — first argument moves left of the function name. One dispatch rule unifies them:

1. Look up the function name as a slot on the first argument (prototype chain) → operator overloading
2. Fall through to global scope → UFCS

```vo
a + b        // sugar for a.+(b)
a.f(b)       // looks up f on a, falls through to global f(a, b)
f(a, b)      // pure prefix — global only
```

Operator overloading and UFCS fall out as free consequences with no new language features.

### Candidate C — FFI descriptor as first-class value

| System         | Approach |
|----------------|----------|
| LuaJIT FFI     | C type declaration strings parsed at runtime |
| Python ctypes  | Type objects constructed imperatively |
| Zig `@cImport` | Compiler-level C header parsing |
| VO `$$`        | Hash descriptor,  a first-class value in the language itself |

FFI binding specs are composable, passable, inspectable, because they are just hashes. Not a special case.

### Anticipated reviewer challenges

**"Is this just lambda calculus restated?"**

Related, but the direction is reversed:

|           | Lambda calculus | VO |
|---        |---              |---|
| Primitive | Function (λ) | Hash (data) |
| Data representation | Church encodings — functions pretending to be data | Real data structures used as data |
| Practical programmability | No | Yes |
| Direction | Functions → data (derived) | Data → functions (stored in hashes) |
| Claim | Theoretical: you *can* derive everything | Practical: you can derive enough that the rest become libraries |

The closer formal comparison is Abadi & Cardelli's object calculus (1996). Lambda calculus is a theoretical claim about expressiveness; VO is a practical claim about language design economy.

### Paper structure

1. Introduction — one primitive; what emerges?
2. Hash as universal primitive — objects, modules, namespaces, FFI specs
3. Accidental homoiconicity — AST-as-hash, `parse`, `eval`, macro system as library
4. Unified dispatch — dot, infix, UFCS as one rule; operator overloading free
5. FFI descriptor as first-class value
6. Related work — Lisp, Lua, Rebol, Self, Smalltalk, Haskell, object calculi
7. Discussion — what was surprising; what the design rules out; future work

## Paper 2 Theory: hash/feature-structure rewriting as a computational model

**Venue**: Theoretical computer science; possibly LICS, FSCD, or as a workshop paper at SPLASH
**Readiness**: Speculative — requires formal development (confluence proofs, expressiveness results)
**Claim type**: Theory / formal methods

### The model

VO's three primitives — hash, iteration (`>>`), callable — define a computational model: **hash rewriting**. A rule is a callable that takes a hash and returns a transformed hash. A rewriting engine applies rules until fixpoint — writable in VO itself.

### Is this just tree rewriting?

Largely yes — a hash is a tree node with labelled children. The distinction:

- **No fixed schema** — hashes are open (keys added/removed at runtime); standard TRS operates over fixed-arity signatures. This is closer to **feature structures** (PATR-II, HPSG) than standard TRS.
- **Keys as first-class data** — `>>` iterates over keys without knowing them in advance
- **Unification-friendly** — feature structures unify naturally; fixed-arity terms do not

Honest reframing: hash rewriting is **feature-structure rewriting** — well-studied in computational linguistics, less studied in PL theory.

### The deep consequence

Once the AST is exposed as hashes, VO's evaluator is itself a hash rewriting system. The interpreter becomes describable in its own terms — a metacircular evaluator via hash rewriting rather than S-expression reduction.

### Relation to established models

| Model | Primitive | Relationship |
|-------|-----------|-------------|
| Lambda calculus | Function application | Direction reversed; functions derived from data in VO |
| Term rewriting (TRS) | Tree pattern match | Hash rewriting is TRS over open-schema feature structures |
| Feature structures (PATR-II, HPSG) | Attribute-value matrices | Closest formal match |
| Chemical Abstract Machine (CHAM) | Multiset reactions | Hash rewriting is CHAM with structured (keyed) molecules |
| Interaction nets (Lafont, 1990) | Pairwise node interaction | Turing complete; well-studied; hash rewriting is less constrained |
| Genetic Programming (Koza, 1992) | Trees pruning/grafting each other | Same destination; VO arrives there from pragmatic language design |

### Formal questions to answer

- Confluence (Church-Rosser equivalent) — does rewriting order affect the final result?
- Termination conditions — which rule sets reach fixpoint?
- Expressiveness — Turing completeness; relation to TRS and feature-structure rewriting
- Categorical treatment — trees as objects, transformations as morphisms

### Shape as computation (speculative extension)

A world of trees coexist. Nodes and leaves are hashes — some members are code. One tree can be the rules to traverse another. Primitive operations — **prune, graft, twist, rename** — transform trees. The final shape of the tree is the output. Computation is transformation of structure into structure, not extraction of a value from structure.

| Operation | VO hash equivalent    |
|-----------|-----------------------|
| Prune     | `without(h, key)`     |
| Graft     | `merge(h, subtree)`   |
| Twist     | key reordering        |
| Rename    | `rename(h, old, new)` |
| Traverse  | `>>`                  |

The well-occupied prior art (GP, interaction nets, graph reduction) means the geometric framing must be narrowed to: VO arrives at this model as an accidental consequence of one pragmatic design decision, not as a deliberate theoretical goal.

---

## Paper 3 

### Title Systems: cache-aware AST layout for tree-walking interpreters

**Venue**: VEE (Virtual Execution Environments), ISMM (Memory Management), or DLS
**Readiness**: Implementable now — a few hundred lines of C++; benchmarks completable before Paper 1
**Claim type**: Systems / performance

### The idea

At runtime, query the CPU's L1/L2 cache sizes. Instrument the interpreter to count visits to each AST node. After warmup, compact the hot subgraph into a contiguous arena sized to fit in cache. Re-run. Hot path pointer chasing hits cache (~4 cycles) rather than main memory (~200 cycles).

### The gap

Research confirms no existing tree-walking interpreter applies this technique. It sits between three bodies of work that have not been combined:

| Prior work | What it does | What it lacks |
|-----------|-------------|--------------|
| AST flattening (Sampson, Cornell) | Static contiguous packing; ~1.5× speedup | Not runtime-driven; not cache-size-aware |
| Self-optimising ASTs (Würthinger / Truffle) | Structural rewriting for dispatch reduction | Addresses dispatch, not layout |
| Hot path B+-tree layout (VLDB 2023) | Runtime hot-path compaction for database trees | Applied to B+-trees, not AST interpreters |

### Why the gap exists

The standard path goes tree-walking → bytecode VM, treating tree-walking as a throwaway phase. The bytecode VM gets cache locality for free (flat instruction array) so nobody optimised the tree-walking stage. The GC community solved adjacent problems with different motivation. The database community reached the technique independently. JIT research absorbed all remaining attention. The gap exists not because the idea is hard, but because the road everyone travelled bypassed it.

### Enabling change: arena allocation

Replace individually heap-allocated `std::shared_ptr` AST nodes with an arena allocator — all nodes in a contiguous slab, referenced by index. Compaction becomes array permutation. Improves baseline performance independent of tracing.

### Expected gains

2–5× over scattered allocation for hot inner loops. Comparable to a bytecode VM for hot paths, without discarding the AST or losing homoiconicity.

### Performance ladder

```
Tree-walking, scattered AST (current)
    ↓
Tree-walking, cache-aware AST layout      ← this paper
    ↓
Bytecode VM
    ↓
JIT
```

### Paper structure

1. Introduction — pointer chasing as the tree-walking bottleneck
2. Background — cache hierarchy; tree-walking interpreters; prior layout work
3. Approach — arena allocation; visit counting; hot subgraph identification; compaction
4. Implementation — in VO interpreter (C++); cache size query; threshold selection
5. Evaluation — benchmarks: scattered vs static flat vs runtime-guided; across workload types
6. Discussion — interaction with homoiconicity; composability with JIT; limitations
7. Related work — AST flattening, Truffle, B+-tree layout, copying GC, PGO

---

## Paper 4 — Architecture: shared profiling across staged runtimes

**Venue**: OOPSLA, VM workshops (MoreVMs), or ManLang
**Readiness**: Requires Paper 3 (cache-aware layout) + bytecode VM to exist first
**Claim type**: Runtime architecture / VM design

### The claim

A tree-walking interpreter, when instrumented for cache-tier layout, produces a profile that is directly consumable by all subsequent compilation stages — making the interpretation phase a **zero-overhead profiling pass** rather than a throwaway prototype.

> One instrumentation pass. Three consumers. No additional profiling cost.

### The architecture

```
Instrumented tree-walking interpreter
         ↓  visit counts, type observations, branch frequencies
         ├─→  L1/L2/L3 cache-tier AST layout    — immediate speedup (Paper 3)
         └─→  Profile export
                    ↓
             Bytecode compiler
             + hot callable reordering
             + aggressive inlining of hot sites
             + branch layout for prediction
                    ↓  + profile
                  JIT
             + monomorphic inline caches
             + type-specialised dispatch
             + register allocation for hot vars
```

Each stage consumes the same profile data differently. The profiling cost is amortised across every subsequent optimisation stage.

### What each stage gets from the profile

| Stage | Profile data consumed | Optimisation applied |
|-------|-----------------------|---------------------|
| AST layout | Visit counts per node | Cache-tier compaction (L1/L2/L3) |
| Bytecode compiler | Hot callable list; branch frequencies | Code reordering; inlining; branch layout |
| JIT | Type observations; call site monomorphism | Specialised dispatch; inline caches; register hints |

### How this differs from existing staged runtimes

Staged runtimes with profile feedback already exist — V8 uses Ignition (bytecode interpreter) to profile for TurboFan (optimising JIT). The differences:

1. **The profiling stage is an AST interpreter, not a bytecode interpreter** — the profile is collected at a higher level of abstraction, before any lowering. This means the profile captures semantic structure (which callables are hot, which hash shapes are common) rather than bytecode-level behaviour.

2. **The profiling stage is itself optimised by the profile** — the cache-tier layout improves interpretation speed during the profiling phase, not just afterwards. The profiler bootstraps its own performance.

3. **The cache hierarchy is an explicit parameter** — tier boundaries are determined by runtime hardware queries, not compiled-in constants. The same runtime adapts to different hardware automatically.

### The novel design principle

> **Self-improving profiling**: the instrumentation that collects the profile simultaneously uses that profile to improve the performance of the collection phase itself.

This is distinct from standard PGO (profile collected in one run, optimisation applied in a second build). Here the profile is collected and consumed in the same run, continuously.

### Relation to prior work

| System | Relationship |
|--------|-------------|
| V8 Ignition → TurboFan | Profile fed from bytecode interpreter to JIT — but profiling stage not itself optimised by the profile; no cache-tier layout |
| LuaJIT tracing | Hot traces compiled from bytecode — single stage, no shared profile architecture |
| PyPy meta-tracing | RPython generates JIT from interpreter description — elegant but orthogonal; no cache layout |
| HotSpot tiered compilation | C1 → C2 with profile feedback — closest in structure; operates on JVM bytecode not AST |

### Paper structure

1. Introduction — staged runtimes; the profiling cost problem; the throwaway interpreter
2. Background — cache hierarchy; PGO; existing staged runtimes
3. Architecture — the shared profile; what each stage consumes; self-improving profiling
4. Cache-tier layout — summary of Paper 3 results as the first consumer
5. Bytecode compilation with profile — inlining, reordering, branch layout
6. JIT with profile — inline caches, type specialisation, register hints
7. Evaluation — end-to-end: cold interpreter vs profiled bytecode vs profiled JIT; profile quality vs bytecode-derived profile
8. Discussion — hardware adaptability; bootstrapping cost; limitations
9. Related work

---

## Honest caveats (all four papers)

- **Paper 1** requires the language to be further along before the full chain can be demonstrated in running code
- **Paper 2** is speculative without formal development; confluence and termination proofs are non-trivial
- **Paper 3** is the most immediately actionable and the most clearly novel — the fastest path to publication
- **Paper 4** depends on Paper 3 and a working bytecode VM; strongest if evaluated end-to-end against V8/HotSpot equivalents
- For all four, a well-written blog post or essay may reach a larger relevant audience with less overhead

## Publication order

```
Paper 3  (cache-aware AST layout) ← implement and publish first
    ↓  provides: arena allocator, visit counter, benchmark suite
Paper 4  (shared profiling architecture) ← needs bytecode VM added
    ↓  provides: profile export format, end-to-end pipeline
Paper 1  (language design / emergence) ← needs homoiconicity, unified dispatch
Paper 2  (theory / hash rewriting) ← speculative; pursue in parallel with 1
```
