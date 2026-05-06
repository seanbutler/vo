---
status: PENDING
priority: (MUST, SOONER, LOW, BROAD)
depends: cache-aware-layout.md (Paper 3), homoiconicity.md + unified-dispatch.md (Paper 1)
related: PUBLISH.md
---

# Publication roadmap

Four papers have been identified from VO's design. They have a dependency order — earlier papers provide engineering foundations for later ones.

```
Paper 3  (cache-aware AST layout)          ← implement and publish first
    ↓  provides: arena allocator, visit counter, benchmark suite
Paper 4  (shared profiling architecture)   ← needs bytecode VM added
    ↓  provides: profile export format, end-to-end pipeline
Paper 1  (language design / emergence)     ← needs homoiconicity, unified dispatch
Paper 2  (theory / hash rewriting)         ← speculative; pursue in parallel with 1
```

## Paper 3 — Cache-aware AST layout *(VEE / ISMM / DLS)*

- Arena allocator replacing `std::shared_ptr` heap allocation
- Visit counter per AST node
- Runtime cache size query (L1/L2/L3)
- Hot subgraph compaction into cache-tier regions
- Benchmark: scattered vs static flat vs runtime-guided
- Most immediately actionable; confirmed gap in literature
- See `cache-aware-layout.md` for engineering steps

## Paper 4 — Shared profiling architecture across staged runtimes *(OOPSLA / MoreVMs / ManLang)*

- Depends on Paper 3 + bytecode VM
- Profile export format (visit counts, type observations, branch frequencies)
- Bytecode compiler consuming profile: inlining, reordering, branch layout
- JIT consuming profile: inline caches, type specialisation, register hints
- Key claim: self-improving profiling — the profiler bootstraps its own performance

## Paper 1 — Language design: emergence from one primitive *(Onward! Essays / DLS)*

- Depends on homoiconicity (`parse`/`eval`) being implemented — see `homoiconicity.md`
- Depends on unified dispatch (dot/infix/UFCS) being implemented — see `unified-dispatch.md`
- The emergence chain: one primitive → many mechanisms at no additional conceptual cost

## Paper 2 — Theory: hash/feature-structure rewriting *(LICS / FSCD / workshop)*

- Speculative; pursue in parallel with Paper 1
- Requires: confluence proofs, termination conditions, expressiveness results
- May produce a formal foundation beneath the practical contributions of Papers 1 and 3/4

See `PUBLISH.md` for full details on each paper.
